# Unreal 寻路系统介绍

## Navmesh

Navmesh：在开源项目recastnavigation上修改而来，可以在源码中查看到Recast和Detour的源码

Recast：基于将场景内碰撞，生成用于寻路的导航网格
Detour：基于导航网格进行寻路

UE的架构是支持自己定制寻路方式的（不同ANavigationData），但是内部只提供了Navmesh这一种方式。

## Nav Data是什么

在 Build Path 时，World 内会为每一个 Agent 生成一份 NavigationData，用于这种类型角色的寻路。具体流程是：A 在寻路时，根据（PreferredNavData、AgentRadius、AgentHeight）找到最符合的 Agent，然后使用该 Agent 对应的 NavigationData，进行路径搜索，最后找出 N 个点，就是寻找到的路径，然后 AI 就按照这条路径操纵移动组件进行移动（移动流程参考UE4：AI‘s MoveTo——代码分析）

而每个 Agent 有自己的 Nav Data Class，也就是说同样体型不同 Nav Data Class 的情况下需要两个 Agent，以飞行为例，我们需要两个 Agent，一个Walk，一个Fly，当需要进行飞行寻路时，就需要找到 Fly 这个 Agent。而 Fly Agent 生成的 NavigationData，就是用于飞行寻路的基础数据。

## Agent 的挑选

在项目设置内，Navigation System 内可以配置一些Agents

在寻路过程中，使用移动组件内的配置去匹配所有Agents

```cpp
const ANavigationData* UNavigationSystemV1::GetNavDataForProps(const FNavAgentProperties& AgentProperties) const
```

匹配的规则：

首先判断 Preferred Nav Data 是否匹配：

然后用 AgentRadius 和 AgentHeight 进行匹配（默认-1，表示使用胶囊体），挑出最匹配的那个

```cpp
const ANavigationData* UNavigationSystemV1::GetNavDataForProps(const FNavAgentProperties& AgentProperties) const
```

单项更优判断：较大的优先（Agent 比目标大，至少不会出现卡住的情况），都比目标大或者小的情况下，较接近的优先
若不符合两项都较优或相同，且之前的Agent不合法（两项有一项是小于目标的），则先使用半径判断，若相同再用高度判断，只有有一项是较优则替换
所以，Agent的设置和实际的相同或者大一些
注意这里两项相同的情况也会替换，而List本身是项目设置内的逆序，所以默认的Agent排在上面

```cpp
for(TArray<FNavAgentProperties>::TConstIterator It(AgentPropertiesList); It; ++It)
{
	const FNavAgentProperties& NavIt = *It;
	const bool bNavClassMatch = NavIt.IsNavDataMatching(AgentProperties);
	if (!bNavClassMatch)
	{
		continue;
	}
	// 配置超过AI多少（默认正的绝对比负的好）
	ExcessRadius = NavIt.AgentRadius - AgentProperties.AgentRadius;
	ExcessHeight = bSkipAgentHeightCheckWhenPickingNavData ? 0.f : (NavIt.AgentHeight - AgentHeight);
	// 刚刚好、当前为正最佳为副、同符号接近的
	const bool bExcessRadiusIsBetter = ((ExcessRadius == 0) && (BestExcessRadius != 0))
		|| ((ExcessRadius > 0) && (BestExcessRadius < 0))
		|| ((ExcessRadius > 0) && (BestExcessRadius > 0) && (ExcessRadius < BestExcessRadius))
		|| ((ExcessRadius < 0) && (BestExcessRadius < 0) && (ExcessRadius > BestExcessRadius));
	const bool bExcessHeightIsBetter = ((ExcessHeight == 0) && (BestExcessHeight != 0))
		|| ((ExcessHeight > 0) && (BestExcessHeight < 0))
		|| ((ExcessHeight > 0) && (BestExcessHeight > 0) && (ExcessHeight < BestExcessHeight))
		|| ((ExcessHeight < 0) && (BestExcessHeight < 0) && (ExcessHeight > BestExcessHeight));
	// Valid 认为是不超过配置
	const bool bBestIsValid = (BestExcessRadius >= 0) && (BestExcessHeight >= 0);
	// 是否和Best相同
	const bool bRadiusEquals = (ExcessRadius == BestExcessRadius);
	const bool bHeightEquals = (ExcessHeight == BestExcessHeight);

	bool bValuesAreBest = ((bExcessRadiusIsBetter || bRadiusEquals) && (bExcessHeightIsBetter || bHeightEquals));
	if (!bValuesAreBest && !bBestIsValid)
	{
		// 如果之前不是Valid，若半径更好，或者半径相同时高度更好，则更新
		bValuesAreBest = bExcessRadiusIsBetter || (bRadiusEquals && bExcessHeightIsBetter);
	}

	if (bValuesAreBest)
	{
		BestFitNavAgent = NavIt;
		BestExcessHeight = ExcessHeight;
		BestExcessRadius = ExcessRadius;
	}
}

if (BestFitNavAgent.IsValid())
{
	NavDataForAgent = AgentToNavDataMap.Find(BestFitNavAgent);
	NavDataInstance = NavDataForAgent ? NavDataForAgent->Get() : nullptr;
}

```

## NavLinkProxy是什么

有一些游戏机制（例如传送门），使得我们的路径没法使用单纯自动生成的寻路网格，需要增加一些处理。这种情形下我们会用到NavLinkProxy，将两个点之间联通（可选单向），我们的寻路就会将这两个点联通。

当AI到达点A时，会触发NavLinkProxy的Receive Smart Link Reached，这里我们可以自定义AI所需要触发的函数，例如响应传送。

## 寻路算法

在多边形的基础上使用A*找出最优路径（N个连通的多边形），然后用拉绳法找出内部的最短路径，这部分很多文章都有提到，这里不再赘述。主要讲一下unreal的实现。

```cpp
FPathFindingResult UNavigationSystemV1::FindPathSync(FPathFindingQuery Query, EPathFindingMode::Type Mode)
// 有两种寻路，FindPath 和 FindHierarchicalPath，第二种是增加的Z轴移动的限制
	if (Mode == EPathFindingMode::Regular)
		Result = Query.NavData->FindPath(Query.NavAgentProperties, Query);
			return (*FindPathImplementation)(AgentProperties, Query);
	else // EPathFindingMode::Hierarchical
		Result = Query.NavData->FindHierarchicalPath(Query.NavAgentProperties, Query);

// 为了支持其它寻路方式，具体的寻路需要每个不同的Navmesh自己实现，也就是子类设置FindPathImplementation这个函数指针
typedef FPathFindingResult (*FFindPathPtr)(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);
FFindPathPtr FindPathImplementation;
FFindPathPtr FindHierarchicalPathImplementation; 

// 在NavMesh构造的时候，将对应的函数设置进入，可以看到，NavMesh没有特写FindHierarchicalPath
ARecastNavMesh::ARecastNavMesh(const FObjectInitializer& ObjectInitializer)
	FindPathImplementation = FindPath;
	FindHierarchicalPathImplementation = FindPath;
```

```cpp
FPathFindingResult ARecastNavMesh::FindPath(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
	// 之前会根据Agent的信息，选择对应的NavData，这份NavData在Recast这个寻路体型中，就是ARecastNavMesh
	const ANavigationData* Self = Query.NavData.Get();
	const ARecastNavMesh* RecastNavMesh = (const ARecastNavMesh*)Self;
	// 调用寻路，这里过了一层FPImplRecastNavMesh* RecastNavMeshImpl
	// 官方是意思是为了让recast（开源寻路项目）对其它引擎代码分离，内部的dtNavMesh* DetourNavMesh是核心寻路数据
	Result.Result = RecastNavMesh->RecastNavMeshImpl->FindPath(Query.StartLocation, AdjustedEndLocation, Query.CostLimit, *NavMeshPath, *NavFilter, Query.Owner.Get());

```

## 寻找多边形路径

```cpp
// Detear 寻找路径
dtStatus dtNavMeshQuery::findPath(dtPolyRef startRef, dtPolyRef endRef, const dtReal* startPos, const dtReal* endPos, const dtReal costLimit, const dtQueryFilter* filter, dtQueryResult& result, dtReal* totalCost) const
	// A*的H值
	const dtReal H_SCALE = filter->getModifiedHeuristicScale();
	// 起点
	m_openList->push(startNode);
		// 数组模拟的二叉树，实现的小顶堆
		m_size++;
		bubbleUp(m_size-1, node);
	while (!m_openList->empty())
		dtNode* bestNode = m_openList->pop();
		// 终点理论值最优（G+H）
		if (bestNode->id == endRef)
			lastBestNode = bestNode;
			break;
		unsigned int i = bestPoly->firstLink;
		// 遍历所有相邻的多边形，dtLink是表示当前多边形对于相邻多边形的link，对保存相邻的多边形，也就是link.ref
		while (i != DT_NULL_LINK)
			const dtLink& link = m_nav->getLink(bestTile, i);
			dtPolyRef neighbourRef = link.ref;
			// 判断是否是来时的边
			if (!neighbourRef || neighbourRef == parentRef
				// 回溯处理
				|| !filter->isValidLinkSide(link.side))
				continue;
			// 根据ref找到tile和poly
			m_nav->getTileAndPolyByRefUnsafe(neighbourRef, &neighbourTile, &neighbourPoly);
				decodePolyId(ref, salt, it, ip);
				*tile = &m_tiles[it];
				*poly = &m_tiles[it].polys[ip];
			// 判断多边形和Link的合法性，这一步应该可以定制，例如哪些区域在这次寻路中不能通过（高度什么的）
			if (!filter->passFilter(neighbourRef, neighbourTile, neighbourPoly) || !passLinkFilterByRef(neighbourTile, neighbourRef))
				continue;
			// 如果该多边形已经存在对应的节点，则返回对应的节点，否则新建一个（对象池）
			dtNode* neighbourNode = m_nodePool->getNode(neighbourRef);
			// 当前节点已是close节点
			if (shouldIgnoreClosedNodes && (neighbourNode->flags & DT_NODE_CLOSED) != 0) continue;
			// 计算G，curCost表示两个多边形之间的代价
			curCost = filter->getCost(bestNode->pos, neiPos, parentRef, parentTile, parentPoly, bestRef, bestTile, bestPoly, neighbourRef, neighbourTile, neighbourPoly);
			cost = bestNode->cost + curCost;
			// 使用距离计算H
			heuristic = dtVdist(neiPos, endPos)*H_SCALE;
			// 总代价
			const dtReal total = cost + heuristic;
			// 不是第一次到达，但是当前代价更大
			if ((neighbourNode->flags & DT_NODE_OPEN) && total >= neighbourNode->total) continue;
			// 新点（或者老的点modify，或者更新，篇幅原因省略）
			m_openList->push(neighbourNode);
			// 维护最优部分路径（没到达目标里，挑一个离目标最近的）
			if (heuristic < lastBestNodeCost)
				lastBestNodeCost = heuristic;
				lastBestNode = neighbourNode;
	// 还原路径
	do
		dtNode* next = m_nodePool->getNodeAtIdx(node->pidx);
		node->pidx = m_nodePool->getNodeIdx(prev);
		prev = node;
		node = next;
	while (node && ++n < loopLimit);
	result.reserve(n);
```

## 处理出Vector路径

```cpp
ENavigationQueryResult::Type FPImplRecastNavMesh::PostProcessPathInternal(dtStatus FindPathStatus, FNavMeshPath& Path...) 
	// 处理同一个多边形内部的情况
	if (PathResult.size() == 1 && dtStatusDetail(FindPathStatus, DT_PARTIAL_RESULT))
	else
		PostProcessPath(FindPathStatus, Path, NavQuery, QueryFilter, StartPolyID, EndPolyID, Recast2UnrVector(&RecastStartPos.X), Recast2UnrVector(&RecastEndPos.X), RecastStartPos, RecastEndPos, PathResult);

void FPImplRecastNavMesh::PostProcessPath(dtStatus FindPathStatus, FNavMeshPath& Path...)
	// 初始化 Path.PathCorridorCost（到达每个多边形的代价）
	for (int32 i = 0; i < PathSize; i++)
		Path.PathCorridorCost[i] = PathResult.getCost(i);
	// 初始化 Path.PathCorridor（每个多边形）
	for (int i = 0; i < PathSize; ++i)
		Path.PathCorridor[i] = PathResult.getRef(i);
	
	if (Path.WantsStringPulling())
		// 拉绳算法
		Path.PerformStringPulling(StartLoc, UseEndLoc);
			FindStraightPath(StartLoc, EndLoc, PathCorridor, PathPoints, &CustomLinkIds);
	else:
		for (int32 Idx = 0; Idx < Path.PathCorridor.Num(); Idx++)
			const dtOffMeshConnection* OffMeshCon = DetourNavMesh->getOffMeshConnectionByRef(Path.PathCorridor[Idx]);
			// 收集link id
			if (OffMeshCon)
				Path.CustomLinkIds.Add(OffMeshCon->userId);

bool FPImplRecastNavMesh::FindStraightPath(const FVector& StartLoc, const FVector& EndLoc, const TArray<NavNodeRef>& PathCorridor, TArray<FNavPathPoint>& PathPoints, TArray<uint32>* CustomLinks) const
	// 找一条平滑路径
	const dtStatus StringPullStatus = NavQuery.findStraightPath(&RecastStartPos.X, &RecastEndPos.X, PathCorridor.GetData(), PathCorridor.Num(), StringPullResult, DT_STRAIGHTPATH_AREA_CROSSINGS);
	for (int32 VertIdx = 0; VertIdx < StringPullResult.size(); ++VertIdx)
		// 转换坐标
		CurVert->Location = Recast2UnrVector(CurRecastVert);
		// 收集link id
		const dtOffMeshConnection* OffMeshCon = DetourNavMesh->getOffMeshConnectionByRef(CurVert->NodeRef);
		CustomLinks->Add(OffMeshCon->userId);
		
dtStatus dtNavMeshQuery::findStraightPath(const dtReal* startPos, const dtReal* endPos, const dtPolyRef* path, const int pathSize, dtQueryResult& result, const int options) const
	// 找起点、终点到多边形上的最近点
	closestPointOnPolyBoundary(path[0], startPos, closestStartPos))
	closestPointOnPolyBoundary(path[pathSize-1], endPos, closestEndPos))
	// 添加起点到路径
	stat = appendVertex(closestStartPos, DT_STRAIGHTPATH_START, path[0], result);
	// 以下部分之后有空再写吧，反正就是一个拉绳
	xxx
	// 添加终点点到路径
	stat = appendVertex(closestEndPos, DT_STRAIGHTPATH_END, 0, result);
```

## Build Path 流程

在Build - Build Path，进入到UNavigationSystemV1::Build()

UNavigationSystemV1::Build()

- void UNavigationSystemV1::SpawnMissingNavigationData()
- void UNavigationSystemV1::RebuildAll(bool bIsLoadTime)
- FNavDataGenerator::RebuildAll

就是对每个NavData，运行NavDataGenerator的RebuildAll函数

## 虚幻Recast dtNavMesh Build流程

## 修改寻路网格

将 DirtyArea 塞入 DirtyAreasController 的 TArray<FNavigationDirtyArea> DirtyAreas

然后在 void UNavigationSystemV1::Tick(float DeltaSeconds) 内，调用

- RebuildDirtyAreas(DeltaSeconds)
- DefaultDirtyAreasController.Tick
- NavData->RebuildDirtyAreas(DirtyAreas)
将这些 DirtyArea 传递给 ANavigationData 的 FNavDataGenerator 处理

以Recast为例，就是将与 FNavigationDirtyArea 相交的 Tile 进行重建

注意，RebuildDirtyAreas(DeltaSeconds) 有一个前提是 IsNavigationBuildingLocked() == false，而当编辑器设置内的 Update Navigation Automatically 为 False 时，NavBuildingLockFlags 会加上 ENavigationBuildLock::NoUpdateInEditor，所以这种情况下除非主动调用 RebuildAll，也就是 Build - Build Path，否则NavData是不会变化的。

## 动态修改寻路

DynamicModifiersOnly
将ProjectSetting内，NavigationMesh的RuntimeGeneration改为DynamicModifiersOnly

针对会移动的障碍，将障碍的StaticMeshComponent的Can Ever Affect Navigation改为False，并且给障碍加上NavModifierComponent。

注意，由于是障碍物，所以需要把NavModifierComponent的AreaClass改为NavArea_Null。

## 优化动态修改寻路 

https://zhuanlan.zhihu.com/p/566846141

## 大世界下寻路网格的使用

1 老办法
直接加载所有的块，然后build path，注意，为了避免之后加载块导致的build path，在编辑器设置内关闭自动更新导航

2 只在周围生成
只在Invoker周围动态生成寻路数据

参考：
https://www.youtube.com/watch?v=DMe536X4IT0
https://www.youtube.com/watch?v=Smuy2d7y7mA&list=PLNTm9yU0zou7kKcN7091Rdr322Qge5LNA&index=47

第一个视频 核心是开启这个选项（还是需要寻路体积的）

下面的参数是更新周期

然后给中心物体（Pawn）加上Navigation Invoker Component

这样子就会在这个物体周围生成寻路数据了

内部有两个Tile Generation Radius和Tile Removal Radius，表示当区域进入Generation 范围就会生成，离开Removal 范围就会删除

第二个视频是说，如果目标太远了，就在周围找一个目标方向的点过去，靠这种方式慢慢接近

3 world-partitioned做法
操作
https://docs.unrealengine.com/5.0/en-US/world-partitioned-navigation-mesh/

https://docs.unrealengine.com/5.1/en-US/world-partition-in-unreal-engine/


## Useful links

https://dev.epicgames.com/documentation/en-us/unreal-engine/optimizing-navigation-mesh-generation-speed-in-unreal-engine?application_version=5.3

https://qiao.github.io/PathFinding.js/visual/

https://zhuanlan.zhihu.com/p/691181077

# Detour寻路

## 寻路过程
我们在寻路时，通常会提供一个起始点坐标和目标点坐标，希望Detour为我们返回一条路径，之后就沿着这条路径移动。Recast生成的Navmesh由多边形组成，而我们需要的是一条由坐标描述的路径，其中还涉及到一个多边形与坐标转换的过程，还是比较复杂的。简单起见，我们可以从UNavigationSystemV1::FindPathSync接口入手进行分析，从接口名称可以看出这是一个同步接口，计算完路径后调用才会返回。

## 得到起点和终点所在的导航多边形

Detour寻路基于导航多边形(以下简称Poly)，因此需要先得到起点坐标和终点坐标所在的Poly，通过dtNavMeshQuery::findNearestPoly接口实现。过程如下：

- 首先对坐标点进行扩张，扩张成一个三维的长方体

坐标点可能并不刚好处于Poly上，因此需要扩张起始点范围，找到附近最接近的Poly。每个UE4 Navigation支持的寻路Agent都可以设置不同的扩张参数，以适应不同角色的体积大小

- 找到最近的Poly

tile中的多边形使用bvtree组织，因此可以快速找到所有与扩张的长方体重叠的poly。之后会在这些Poly中找到离坐标点最近的Poly，作为返回值。

## 使用A*算法得到Poly组成的路径

把起点位于的多边形记为StartPoly，终点位于的多边形记为EndPoly，我们可以找到一条从StartPoly到EndPoly的路径，这只是一条粗略的路径，后续还需要微调。具体逻辑在dtNavMeshQuery::findPath中。

寻路算法为A*，是一种被广泛使用的寻路算法，不一定能找到最短路径，但能快速找到一条相对短的路径。其特点为在Dijkstra算法基础上引入了启发因子，在寻找节点时会偏向那些离终点“看起来更近”的节点。

本文不对A*做过多介绍了。

- 启发函数
启发函数是A*算法的重要部分，Detour中使用了欧几里得距离作为启发函数，即两个坐标之间的三维距离。

- 启发因子
A*算法中的开销公式为：F = G + H*H_SCALE，其中H_SCALE为启发因子，用于控制启发距离的影响程度，可在编辑器中设置，默认为0.999。如果把H_SCALE设为0，则A*退化为了Dijkstra算法。

寻路过程会用到关键的数据结构dtNode，是一个Poly的包装和抽象，其定义如下：

```cpp
struct dtNode
{
	float pos[3];				///< Position of the node.
	float cost;					///< Cost from previous node to current node.
	float total;				///< Cost up to the node.
	unsigned int pidx : 30;		///< Index to parent node.
	unsigned int flags : 2;		///< Node flags 0/open/closed.
	dtPolyRef id;				///< Polygon ref the node corresponds to.
};
```


属性说明：

Pos: Poly的位置。由于Poly是一个片，因此需要一个坐标来代表这个Poly，Pos可能是边缘的点，也可能是中间的点，执行A*算法时会动态赋值，具体可见下文

cost：从StartNode到前一个Node花费的开销，这个开销是确定的，不带启发距离

total：路径经过这个Node到终点需要花费的总开销，为cost+启发距离

pidx：记录的前一个节点的id，用于得到一条由Node形成的路径

flags：这个Node的状态，0=未遍历过，1=已在openList中，2=已在closeList中

id：该Node对应Poly的id

在了解节点数据结构后，让我们简单看一下寻路算法的执行过程：

- 创建StartPoly对应的dtNode，把pos设置为起始点坐标，由于是寻路的第一个节点，因此其total开销完全由启发距离决定，然后把StartdtNode插入到最小堆openList中。
- 不断从openList中取当前total最小的dtNode，作为当前节点处理，如果当前已经是终点所在的Poly，则表明找到了，退出openList处理流程。
- 对于当前dtNode，更新与它对应Poly相邻的dtNode，寻找开销更少的路径，同时更新openList，这最是关键的一步。
- 找到EndPoly后，就根据Node的pidx来构造出一条Poly组成的路径，并返回
- 如果没找到EndPoly，或者onpenList处理循环数超过阈值，则会把之前与EndPoly启发距离最小的Node对应路径作为结果返回，即返回部分路径。

## 经过一个Poly的开销如何计算?

这里有个重要的问题，就是Poly并不是一个抽象的点，当我们决定要经过一个Poly时，如何计算经过这个Poly所花费的开销？我们在这个Poly中的移动路径是怎么样的？

Detour的解决方法为取Poly边的中点进行连接，计算距离，因为这个步骤本身也是得到粗略路径，因此这种近似也可以接受。

当我们处理当前Node，对OpenList中NextNode距离做紧缩时，其实需要ParentNode、CurrentNode、NextNode三个Node的信息。首先取ParentNode和CurrentNode连接的边，计算边的中点，然后取CurrentNode和NextNode连接的边，计算边的中点，之后计算两个中点的欧几里得距离即可。这个距离就是经过当前Node所要经过的距离。

OffMesh Link如何处理？

OffMesh Link是跳跃的，此时取边中点方案就不适用了。其实OffMesh Link也是以Poly数据结构储存在导航数据中的，它是一种特殊的Poly，可以始用正常计算Poly连接边中点的流程处理它，中点为OffMeshLink一端的坐标位置。

## 经过Poly的开销

经过一个Poly的开销并不完全等同于距离，总开销=切换Poly类型固定开销+经过距离*距离开销系数。每个Poly可以设置类型，最多可以有64种，我们可以为每种类型设定切换开销和距离开销系数，比如沼泽类型，平地道路类型。

但是，计算启发距离时不会考虑Poly的开销，仅考虑距离因素。

### 使用漏斗算法平滑路径

UE4默认开启了路径平滑选项，Detour会使用漏斗算法(也称为拉绳算法)对Poly组成的路径进行处理，得到一个由坐标组成的平滑路径，看起来更加自然。如下图所示，图中把个相邻Poly的顶点用线连了起来，是为了表示更直观，并不是Poly都是三角形。

漏斗算法的思想如下：

假设我们得到了一条由Poly组成的路径，其实就是Poly的边组成的路径，为图中的黄色线段。我们会维护“漏斗”的顶点和左右两条边，可以再图A中看到，两条边之间的区域是我们可以从漏斗顶点直线可达的最大区域，漏斗外的区域会没有Poly，不可行走。

执行漏斗算法时用到的Poly坐标都是xy平面上的二维坐，不考虑Poly的高度，这也是因为Recast主要用于二维平面上的寻路。

初始时可以把左右两条边先连接到第一个Poly边上。接下来，需要依次移动左右两条表，按顺序处理路径上的Poly。

首先移动左边连接到下一个Poly的左边顶点，发现移动后的左边介于移动前左右两条边之间，表明左边的移动是可接受的，于是更新左边，这个Poly处理完成，如图B。接着移动右边，处理下个Poly，移动右边连接到Poly的右断点，发现移动后右边也介于移动前左右两条边之间，因此这个移动也被接受了，如图C。按照相同的方式，图D中左边的移动也被接受。

但是当在E中移动右边时，发现移动后右边不再介于移动前的两边之间，而是外扩了，外扩后漏斗间出现了无Poly覆盖区域，因此这次右边移动被拒绝。此时会再尝试移动左边，如果移动合法就会继续按照之前的流程处理接下来的Poly。但图F中左边移动后穿插到了右边的右侧，因此这次移动也被拒绝，这时发现漏斗已经无法继续往下扩展了，需要先设置一个中间点，作为新的漏斗顶部，然后把遍历Poly的序号回退到这个中间路点位置，从这个Poly开始扩展新的漏斗，因此会有循环往复的操作。

OffMeshLink怎么处理？

因为OffMeshLink也由Poly数据结构表示，所以也可以使用漏斗算法，但是由于Link只是一个点，完全套上漏斗的“左边”“右边”不太合适，因此当ToPoly为OffMeshLink时，会直接把当前的left和right点经过一些调整，生成一个新的漏斗顶，并从这里开始重新执行漏斗算法。注意此时生成新漏斗顶并不是因为异常情况，所以不存在回退执行。

具体代码可见dtNavMeshQuery::findStraightPath函数。
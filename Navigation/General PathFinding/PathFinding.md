# Path Finding

https://qiao.github.io/PathFinding.js/visual/

https://zhuanlan.zhihu.com/p/656641483

## Introduction

本小结会深入浅出的分析常用的AI寻路技术，以及对比各家寻路技术方案的优劣点，然后分析目前应用最广泛的寻路技术，剖析其寻路技术的生成和寻路算法部分，以及在大世界的应用和优化等。

如何自己实现寻路？一般寻路技术的主要用法就是给定一个初始出发坐标StartPos，然后再给定一个目标结束点坐标EndPos，寻路算法主要做的就是找到从StartPos到EndPos的可达最短路径。 假如我们没有任何寻路技术背景，自己来实现寻路算法你会怎么做？那么这里就有2个关键的点需要注意，首先构成这个路径的数据是什么？也就是寻路数据是什么格式，其次采用什么算法去寻路。按照我们大学学习到的最基础的遍历算法，我们都知道深度优先遍历和广度优先遍历。我们在假设寻路数据是最简单的网格的情况下，使用广度优先算法或者朝着目标方向的深度优先算法最终都能从StartPos找到EndPos，那么我们自己实现最简单的寻路算法就实现了。

从以上的例子延伸出几个问题，如果没有特殊的优化方法，只是无脑的使用广度优先遍历或深度优先遍历，其搜索效率都不是很高，且这种情况的广度优先遍历可能会导致内存太大。另一个问题是构成我们的寻路数据的格式可能不一定是简单的网格，也许还有其他更复杂的格式比如多层网格或NavmeshPoly，不管什么格式，游戏中从地形编译生成该寻路数据都是一个较大的课题，比如需要体素化等等。寻路算法有很多种，在正式谈A*算法之前，我们先回忆一下书上学的一些最短路径算法。

## 基本定义

图是由顶点的有穷非空集合和顶点之间边的集合组成，通常表示为：G(V,E)。

其中G表示一个图，V是图G中定点的集合，E是图G中边的集合

带权的图统称为网

有很少条边的被称为稀疏图，反之称为稠密图

存储方式：
1. 邻接矩阵 （适合稠密图）
2. 邻接表：
字典的字典（字典由哈希表实现）。或是链表的数组（结点存入数组，并对及节点的孩子进行链式存储。
建立需要O(v+e)的时间复杂度

有向图中，若想通过邻接表知道有哪些顶点可以连向该顶点（入度），复杂度为O(n)。解决：4. 十字链表
无向图中，若想通过邻接表删除某条边会比较繁琐，因为要找两次。解决：5. 邻接多重表

3. 边集数组：
两个一维数组，一个存储顶点信息，一个存储边的信息。


一些算法:

1. BFS/DFS O(V+E)
    
    有向无环图（DAG）可以直接拓扑排序，再按照这个顺序DP求最短路，复杂度O(V+E)，不用到用Dijkstra

2. Dijkstra O(V^2), drops down to O(V + ElogV) with fibonacci-heap

    Dijkstra's algorithm only works on graphs with no cycles, or on graphs with a positive weight cycle.

    "带权重的BFS" 以图中的一个起始节点为基础，通过逐步扩展到其他节点来计算到每个节点的最短路径。Dijkstra算法通过不断更新节点的最短路径值，逐步扩展到其他节点，直到找到目标节点或者遍历完所有节点。它保证了在加权图中找到最短路径，但对于存在负权边的图，Dijkstra算法不适用。Dijkstra算法在路径规划、网络路由等领域广泛应用，它能够高效地找到最短路径，并且可以通过优先队列等数据结构来提高算法的效率。

3. Greedy BFS O(V+E)

    Not good with obstacles. Picks the best node based on some rule called "heuristics". 按照离目标距离的远近排序（而不是Dijkstra的当前最短路径），所以是较优，但不总是最优。

4. bellman-ford O(VE)

    原理是对图进行V-1次松弛操作。实现简单，但复杂度过高。
    Unlike the Dijkstra algorithm, this algorithm can also be applied to graphs containing negative weight edges . However, if the graph contains a negative cycle, then, clearly, the shortest path to some vertices may not exist (due to the fact that the weight of the shortest path must be equal to minus infinity); however, this algorithm can be modified to signal the presence of a cycle of negative weight, or even deduce this cycle.

5. floyd-warshall O(V^3)
    
    是一种用于解决所有节点对最短路径的动态规划算法。它能够在有向图或带权图中找到任意两个节点之间的最短路径。Floyd算法的时间复杂度为O(n^3)，其中n是节点的数量。它适用于解决稠密图或有向图中的最短路径问题，但对于大型图可能会变得相对较慢。Floyd算法在网络路由、图论等领域有广泛的应用，它能够找到任意两个节点之间的最短路径，并且可以处理负权边的情况。然而，Floyd算法的空间复杂度较高，因为需要存储所有节点对之间的距离信息。

6. SPFA（Shortest Path Faster Algorithm）

    是一种用于解决单源最短路径问题的算法，它是对Bellman-Ford算法的一种优化，SPFA算法的优化在于使用了队列来选择下一个要处理的节点，而不是像Bellman-Ford算法那样遍历所有节点。这样可以减少不必要的重复计算，提高算法的效率。SPFA算法适用于解决带有负权边的图中的最短路径问题，但对于存在负权环的图，SPFA算法会进入无限循环。因此，在使用SPFA算法时，需要注意检测负权环的存在。SPFA算法在网络路由、图论等领域有广泛的应用，它能够高效地找到单源最短路径，并且可以处理负权边的情况。

7. Theta（Theta Star）
    是一种对A算法的一种改进和优化。Theta算法在A算法的基础上引入了"theta"角度的概念，用于判断两个相邻节点之间是否可以直接连线，而不需要沿着网格或图的边缘行走。这样可以减少路径的节点数，提高路径规划的效率。接下来介绍常用的寻路算法A*
# 游戏AI

本文所讲为上篇中的狭义AI范畴

## 架构

### 概述

游戏 AI 的架构是指为实现游戏中智能行为而设计的整体系统结构。一个良好的游戏 AI 架构需要具备高效性、可扩展性和灵活性等，以便处理各种复杂的游戏场景和交互。

典型的游戏 AI 架构一般包括如下模块（或者叫组件或系统）：感知、决策、行为、动作。其中动作和行为也有合并到一起的，但这里分开为2套，原因下面再说。也有更多组件模块的，如基于强化学习的 AI 架构还包括学习和适应组件等，但本文还是尽量关注在较为典型的游戏架构上。

### 感知

感知模块负责收集游戏世界的信息，并将这些信息传递给 AI 代理。

从技术角度来说，感知模块为其他模块提供数据，数据哪里来？从游戏世界里获取。

感知模块获取数据的方式一般有两种：“说人话”：思考和反馈（这是真的从人的角度上说的话）。用机器（程序）的话说则分别对应：拉取和推送，或者叫轮询和事件。如果读者是程序员的应该对这些概念相当熟悉。翻译为通用语则分别对应：AI 主动从游戏世界获取数据；游戏世界主动给 AI 推送数据。

感知模块的实现上，很多方案都是主要借助黑板（Blackboard，BB，行为树常用）或类似的数据容器（如 GOAP 和 HTN 的 WorldState ）来做到的。

### 决策

决策模块负责根据感知到的信息做出行为选择。

这个其实才是本文想要分享的内容的重点，后面会提到，虽然我们说的是游戏 AI 技术方案或者架构，应当包含所有本段所述的模块，各模块都有其重要性，但其他模块其实有很大的通用性或复用性，唯决策模块在不同技术方案下差异或者说特色很明显，后面我们重点讨论这个。

### 行为

行为模块负责将决策模块选定的高层次行为转化为具体的行动计划。一旦决策模块选择了一个行为，行为生成模块负责执行具体的动作。这包括动画控制、路径规划和动作指令等。

可以理解为从“要做什么”到“如何做”的桥梁。

### 动作

动作模块负责具体执行和管理由行为生成模块指派的低层次动作，包括移动、攻击、互动等。

这一层是直接与游戏引擎的物理和动画系统交互的部分。你也可以直接理解为就是对各种引擎底层API的调用吧，比如调用动画播放接口，调用移动接口啥的。

以上也可以看出，动作和行为两个模块的差异，他们在抽象层次、职责范围和交互对象等方面都是不一样的。

不过相当多文章把行为和动作两个模块统一为一个的，这个从某些角度来说也是可以的，但本文想把他们分开，这样一定程度上更有利于实际开发时的功能解耦和复用。


## 方案

典型的游戏 AI 架构方案有 FSM、BT、Utility、GOAP、HTN 等，这是本文的重点，他们的概念、原理、示例代码、优劣、适用场合和应用示意等后面详述。

不同的 AI 方案在可控性、AI 自治性（结果的可预测性）、可扩展性、实现的复杂度、是否可规划、性能开销等方面是不一样的，具体会在下文各方案的具体段落细说，文末总结部分也会以表格形式展示出来。可以先看看《Game AI Pro》对各方案的可控性和自治性的展示，有个大概的印象即可：

![](./assets/autonomy.png)


一，GOAP 和 HTN 等均属于 Planning；

二，Utility AI 本身其实也有多种级别，简单的可能比 Planning 更好控制，复杂的则可能比 Planning 更有自治性。

三，自治性或者说 AI 的行为是否难以被预测性、AI 是否难以被掌控，这个并非绝对的是利或者是弊，不同的游戏或需求对这个的要求是不一样的，要辨证去看。

可以在后面的段落中对对应的 AI 方案做了说明有了足够的理解后再来验证这点。

其实也可以叫做 AI 技术架构，但是叫 AI 技术方案更亲民一点，总之本段要说的是本文的核心：游戏 AI 的各种行为决策方案（但这些方案也是包括游戏 AI 的其他模块的，比如感知和行为等）。

### FSM

#### 概念

有限状态机（Finite-State Machine，FSM）又称有限状态自动机（Finite-State Automaton，FSA），简称状态机，是表示有限个状态以及在这些状态之间的转移和动作等行为的数学计算模型，在游戏 AI 中也是一种简单但功能强大的 AI 架构，适用于管理 AI 代理的不同状态及其转换。例如，一个 NPC 可以处于“巡逻”、“追逐”或“攻击”状态，在不同状态之间根据特定条件进行转换。

这东西可能大部分读者都很熟悉，即便是还没工作的学生，也对此有所了解，同时在某些游戏项目或引擎下，FSM 的使用也很常见。

状态（State）（加上各种方案的英文有其必要性，因为这对于理解相关英文的论文、文档以及阅读和编写相关的代码都有帮助）：每个状态表示 AI 的一种特定行为或动作。

转换（Transition）：状态之间的有向边，表示从一个状态到另一个状态的转换条件。

输入（Input）：触发状态转换的条件或事件，例如玩家的位置、时间等。

初始状态（Initial State）：AI 的初始状态，表示开始时的行为。

#### 流程

定义状态：列出所有可能的状态，例如“巡逻”、“追踪”、“攻击”等。

定义转换条件：确定每个状态之间的转换条件，例如“玩家进入视野”、“玩家离开攻击范围”等。

实现状态行为：为每个状态编写具体的行为逻辑，例如在“巡逻”状态下，AI 角色会在预定路径上移动。

实现状态管理：实现状态的转换逻辑，确保在满足转换条件时，能够正确切换状态。

#### 示例

FSM 很常见，实现方式也多种多样，可以定义多个状态后通过 if else 转换，也可以用 OOP 思想封装一下各种状态（业务逻辑里常用），还可以定义为节点（就跟 UE 和 Unity 的动画状态机一样）。

这里直接展示一下《Game AI Pro 3》里的一种“可复用轻量级”实现，虽然轻量，但 FSM 的要素均具备，且状态的切换基于 Transition 实现，一定程度上耦合较低，比较推荐。

```cpp
class State
{
    GameObject *m_pOwner;

public:
    State(GameObject *pOwner)
        : m_pOwner(pOwner)
    {
    }
    virtual ~State() {}
    virtual void OnEnter() {}
    virtual void OnExit() {}
    virtual void OnUpdate(float deltaTime) {}

protected:
    GameObject *GetOwner() const { return m_pOwner; }
};

class StateMachine
{
    typedef pair<Transition *, State *> TransitionStatePair;
    typedef vector<TransitionStatePair> Transitions;
    typedef map<State *, Transitions> TransitionMap;
    TransitionMap m_transitions;
    State *m_pCurrState;

public:
    void Update(float deltaTime);
};

class Transition
{
    GameObject *m_pOwner;

public:
    Transition(GameObject *pOwner)
        : m_pOwner(pOwner)
    {
    }
    virtual bool ToTransition() const = 0;
};

class RunAnimationState : public State
{
    AnimationId m_animToRun;

public:
    RunAnimationState(GameObject *pOwner)
        : State(pOwner)
    {
    }
    // Function to load the state definition from XML, JSON,
    // or some other data system. This will fill all the
    // internal data members for this state.
    virtual bool LoadStateDef(StateDef *pStateDef) override;
    virtual void OnEnter() override
    {
        GetOwner()->RunAnimation(m_animToRun);
    }
};

void StateMachine::Update(float deltaTime)
{
    // find the set of transitions for the current state
    auto it = m_transitions.find(m_pCurrState);
    if (it != m_transitions.end())
    {
        // loop through every transition for this state
        for (TransitionStatePair &transPair : it->second)
        {
            // check for transition
            if (transPair.first->ToTransition())
            {
                SetState(transPair.second);
                break;
            }
        }
    }
    // update the current state
    if (m_pCurrState)
        m_pCurrState->Update(deltaTime);
}

```

#### 优点

简单易懂：FSM 的逻辑简单，易于实现和理解。

明确的行为分离：每个状态对应一种具体的行为，便于管理和调试。

性能高：FSM 的状态转换和行为执行通常具有较低的计算开销。这点不容忽视，其实很多看起来强大的方案（我没说你啊，GOAP），性能开销也对应的大了很多，所以到底怎么选取，还得结合具体的需求和环境了）。

#### 挑战

状态爆炸：当状态数量增多或行为复杂度增加时，FSM 的状态和转换条件会迅速增多，导致管理困难。

缺乏灵活性：FSM 的状态和转换条件是固定的，难以应对动态变化或复杂的行为需求。这个也一定程度上会限制 AI 的自治性，导致其比较容易被预测。

难以扩展：增加新的状态或修改现有状态可能会影响整个 FSM 的结构，需要重新设计和实现。

#### 进阶: 分层有限状态机（Hierarchical FSM, HFSM）

HFSM 是 FSM 的扩展，将状态机分层，每层处理不同粒度的行为，可用于处理更复杂的层次结构行为。例如，高层状态机可以管理“战斗”、“逃跑”等大状态，而低层状态机可以管理具体的攻击或防御行为。

分层的思想在其他很多 AI 技术方案上也适用，核心作用大差不大，后面在其他方案详述的时候也会提到。

#### 应用场景

适用于简单、状态较少的行为逻辑。

简单敌人AI：适用于简单的敌人行为，如巡逻、追逐、攻击和闲置等状态之间的切换。

NPC 对话系统：NPC 对话状态管理，可以根据玩家的选择或其他条件切换对话状态。

基础游戏机制：例如开门机制，可以根据玩家的距离和动作来切换开门和关门状态。

小游戏和迷你游戏：适用于那些状态较少、逻辑简单的游戏场景。

业务流程逻辑：此外，FSM 还广泛应用于业务逻辑的编写上，这个跟引擎无关，不论是 Unity 还是 Unreal 等，毕竟都有代码，也需要进行流程控制，比如在启动界面、加载界面、登录界面、主界面、游戏场景等均可以定义为状态，通过 FSM 进行切换。

#### 总结

游戏 AI 基于有限状态机的设计将 AI 的行为划分为若干具体的状态，并在这些状态之间进行切换。

特点： 每个状态代表 AI 的某种特定行为或动作，状态之间通过触发条件进行转换。

优点： 简单、易于实现和理解，适用于简单行为逻辑。

缺点： 对于复杂行为，状态和转换数量会急剧增加，难以维护。


### BT

#### 概念

行为树（Behavior Tree，BT）因其开发范式而流行：只需对 NPC 的动作进行编程，然后设计一个树形结构（通常通过拖放操作）即可创建复杂行为，其叶节点表示动作，其内部节点决定 NPC 的决策。

行为树在图形上表示为有向树，其中节点分为根节点、控制节点或执行节点（任务）。对于每对连接的节点，传出节点称为父节点，传入节点称为子节点。根节点没有父节点，只有一个子节点；控制节点有一个父节点和至少一个子节点；执行节点有一个父节点，没有子节点。在图形上，控制节点的子节点位于其下方，从左到右排列。

行为树的执行从根节点开始，根节点以特定频率向其子节点发送 tick。Tick 是允许执行子节点的启用信号。当允许执行行为树中的节点时，如果其执行尚未完成，则它会向父节点返回状态“正在运行” ，如果已实现目标，则返回“成功” ，否则返回“失败”。

行为树的主要节点类型如下：

根节点（Root Node）：行为树的起点，每次更新行为树时，从根节点开始执行。

控制节点（Control Nodes）：包含多个子节点，用于控制子节点的执行顺序和条件。常见的控制节点有选择节点（Selector）、序列节点（Sequence）和并行节点（Parallel）。

选择节点（Selector）：类似于“或”的逻辑操作，从上到下依次检查子节点，直到找到一个成功的节点。只要找到一个成功的子节点，选择节点就返回成功。

序列节点（Sequence）：类似于“且”的逻辑操作，从上到下依次检查子节点，如果所有子节点都成功，则序列节点返回成功；如果任一子节点失败，则序列节点返回失败。

并行节点（Parallel）：并行执行所有子节点，根据设定的条件来判断成功或失败。例如，可以设定并行节点需要N个子节点成功才返回成功。

叶节点（Leaf Nodes）：行为树的终端节点，执行具体的行为或动作。叶节点通常不会有子节点。

装饰节点（Decorator Nodes）：修饰或限制子节点的行为。例如，重复执行某个行为、在某个条件下执行行为等。

行为树与分层状​​态机有一些相似之处，关键区别在于行为的主要构建块是任务而不是状态。

#### 流程

从根节点开始：每次更新行为树时，从根节点开始执行，根据根节点的类型决定如何处理子节点。

遍历和执行子节点：按照控制节点的逻辑（选择、序列或并行）依次检查和执行子节点。

返回状态：每个节点返回一个状态，通常有三种：成功（Success）、失败（Failure）和运行中（Running）。

状态传播：子节点的状态会影响父节点的状态。例如，如果序列节点的某个子节点失败，整个序列节点返回失败。


#### 示例

假设我们有一个简单的游戏AI，控制一个敌人角色的行为。敌人的行为包括巡逻、追踪玩家和攻击玩家。我们可以构建如下的行为树：

```
根节点
└── 选择节点
    ├── 序列节点
    │   ├── 检查玩家是否在攻击范围内
    │   └── 攻击玩家动作
    ├── 序列节点
    │   ├── 检查玩家是否在追踪范围内
    │   └── 追踪玩家动作
    └── 巡逻动作
```

根节点：开始执行行为树。

选择节点：依次检查每个子节点，直到找到一个成功的子节点。

第一个序列节点（攻击玩家）：检查玩家是否在攻击范围内，如果是，则执行攻击玩家动作。

第二个序列节点（追踪玩家）：如果玩家不在攻击范围内，检查玩家是否在追踪范围内，如果是，则执行追踪玩家动作。

巡逻动作：如果上述所有条件都不满足，则执行巡逻动作。

#### 代码

```python
class Node:
    def run(self):
        raise NotImplementedError

class Action(Node):
    def __init__(self, action):
        self.action = action

    def run(self):
        return self.action()

class Selector(Node):
    def __init__(self, children):
        self.children = children

    def run(self):
        for child in self.children:
            result = child.run()
            if result == "SUCCESS":
                return "SUCCESS"
        return "FAILURE"

class Sequence(Node):
    def __init__(self, children):
        self.children = children

    def run(self):
        for child in self.children:
            result = child.run()
            if result == "FAILURE":
                return "FAILURE"
        return "SUCCESS"

# Example actions
def check_player_in_attack_range():
    return "SUCCESS"  # Replace with actual check logic

def attack_player():
    print("Attacking player")
    return "SUCCESS"

def check_player_in_chase_range():
    return "SUCCESS"  # Replace with actual check logic

def chase_player():
    print("Chasing player")
    return "SUCCESS"

def patrol():
    print("Patrolling area")
    return "SUCCESS"

# Construct behavior tree
behavior_tree = Selector([
    Sequence([
        Action(check_player_in_attack_range),
        Action(attack_player)
    ]),
    Sequence([
        Action(check_player_in_chase_range),
        Action(chase_player)
    ]),
    Action(patrol)
])

# Run behavior tree
behavior_tree.run()
```


#### 优点

直观和易于理解：行为树的层次结构和节点执行流程清晰，便于设计和调试。

模块化和可扩展性：行为树的节点可以独立设计和测试，便于行为的添加和修改。

灵活性：可以通过组合不同类型的节点，设计出复杂且灵活的AI行为。

#### 挑战


节点管理和维护：随着行为树的规模增大，节点的管理和维护可能变得复杂。

调试和优化：尽管行为树直观易懂，但在复杂环境下，调试和优化行为树可能需要较多的测试和调整。

#### 特殊节点

记忆选择节点（Memory Selector）：记忆选择节点会记住上次成功执行的子节点，下次执行时从该节点继续。适用于需要保持行为状态的场景。

记忆序列节点（Memory Sequence）：类似于记忆选择节点，但适用于序列节点。它会记住上次执行的子节点，保证行为的连续性。

权重选择节点（Weighted Selector）：根据子节点的权重值来选择行为，权重越高的子节点越有可能被选择。适用于需要随机性或概率性决策的场景。

时间装饰节点（Time Decorator）：限制某个行为在一定时间内的执行，例如，某个行为每秒只执行一次。

条件装饰节点（Condition Decorator）：根据某个条件来决定是否执行子节点的行为，例如，只有在玩家血量低于50%时才执行治疗行为。

#### 事件驱动行为树

事件驱动行为树通过改变树内部处理其执行的方式，并引入一种可以对事件做出反应并中止正在运行的节点的新类型的节点，解决了传统行为树的一些可扩展性问题。

#### 应用场景


适用于复杂、模块化的行为管理，具有良好的扩展性和可读性。

复杂敌人AI：适用于复杂的敌人行为，可以通过组合不同的行为节点实现复杂的行为模式。

任务和任务链：适用于那些需要执行一系列步骤的任务，如采集资源、建造建筑等。

动态情境应对：行为树可以灵活地处理动态变化的游戏情境，如敌人的动态反应和策略变换。

多层次行为管理：适用于需要多层次管理的 AI 行为，如多层次的巡逻、攻击、防御策略等。

#### 总结

行为树是一种分层的决策结构，通过树状节点组织和管理行为。

特点： 树的叶节点表示具体动作，内部节点（如选择节点、顺序节点）用于控制行为的执行顺序和条件判断。

优点： 模块化设计，行为易于扩展和复用，适合复杂行为的管理。

缺点： 对初学者来说理解和调试较为复杂。

### Utility
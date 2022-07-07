# 基于PBD的模拟的练习
在UE上练习用GPU做物理模拟，尽可能不修改引擎，在插件中做全部，仍在施工中。   
## todo
- [x] 距离约束版的弯曲约束，参考chaos
- [X] 基于运动正交推导的弯曲约束，[参考](https://www.cs.ubc.ca/~rbridson/docs/cloth2003.pdf)
- [ ] 基于迭代次数和帧率的刚性计算，参考chaos
  - [x] 基于迭代次数的计算
  - [ ] 基于帧率和Dt修正
- [ ] Long Range Attachments
  - [ ] GPU上的Mesh Connected Component Label
- [ ] 组件的PhysicsState的摧毁
- [ ] 每个物体独立的重力参数
- [ ] DispatchIndirect
- [ ] 布料渲染时支持法线纹理
  - [ ] 动态计算切线
- [ ] 使用RDG重写
  - [ ] 使用RDG实现一遍gpu radix sort
# 模拟器类
使用了UE提供的WorldSubSystem实现了两个生命周期托管到引擎的单例类，PhyscisScene
和PhysicsSimulator，每一次tick时，PhyscisScene首先会整理各种Object的数据，遍历待加入到GPU场景的物体的列表，通过计算着色器将物体的数据写入到各种buffer中，比如位置buffer，同时根据需求产生约束。   

# 布料
当前使用StaticMesh来生成布料粒子和约束，顶点色的R通道用来标定哪些是固定点。顶点色可以用引擎自带的工具来刷。     

![基于网格生成布料](Img/ClothMeshVertexColorSample-0.png)  

![基于网格生成布料](Img/ClothMeshVertexColorSample-1.png)  

当然使用顶点色并不是最好的方式，理论上物理的数据应该和渲染的数据分开，不过这需要些时间来读UE当前布料资产的代码。  
## 距离约束

约束是使用IndexBuffer生成的，计算着色器的每个线程读取IndexBuffer中的3个值，然后生成3个距离约束。  
这样会有重复的约束生成，因为一个边可能被两个三角形共享。所以我做了一个GPU上的去重操作，大概就是结合排序和求前缀和   

大概流程如下  
* [3, 5, 3, 4, 1, 4, 9, 8, 6, 3, 5, 7, 8, 8, 4, 6, 4, 2, 5, 6] -- 0
* 排序，得到排序后的buffer，  
  [1, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 8, 8, 8, 9] -- 1  
* 对buffer-1 判断一个位置是否是连续相同数据(segment)的边界，其实就是判断左边的数据和自己是否相同   
  [1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1] -- 2
* 对buffer-2 进行归约操作就能得到每个标为1的值的输出位置，进而将buffer压缩成unique的

也是参考了[tensorflow的做法](https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/kernels/unique_op_gpu.cu.h)。  

## 弯曲约束

![弯曲约束](Img/BendingConstraint.png)  
左侧的布料没有弯曲约束，右侧的有  

现在增加了基于距离约束的弯曲约束，参考chaos的实现，简单地约束两个有共享边的三角中除了共享边以外的两点的距离。   

生成弯曲约束的算法和生成距离约束类似，区别在于这一次是寻找那个重复的，也就是说，如果左边的数据和自己相同，则为1，和距离约束的情况相反。当一个边的标识为1时，就将自己所在的三角形的id和buffer中左侧的边的三角形id输出，这样就找到了两个有共享边的三角形。  

## 基于角度的弯曲约束
参考的是chaos使用的方法，是一篇03年的[文章](https://www.cs.ubc.ca/~rbridson/docs/cloth2003.pdf)。  

![弯曲约束](Img/BendingConstraint-Figure-1.png)  

![弯曲约束](Img/BendingConstraint-Algo.png)   

这篇文章的思路并非对约束求梯度，而是基于运动的正交性，来推导出弯曲运动的运动模式，最后得到上面的公式。当前我的实现还有些bug，明显的不稳定。   
从部分视觉效果来看，基于距离的弯曲约束会影响拉伸部分的约束，不方便控制，而基于角度的弯曲约束能够避免这一点。    

# SceneComponent
有了网格体资源之后，场景中用自定义的PrimitiveComponent来显示模拟的物体，自定义vertexfactory来fetch模拟出的顶点位置和法线，这可能带来大量的shader编译 (也是我开发时的困扰点，这很浪费时间)。   

新建一个`YQPhysicsMeshActor`，这会自动创建一个我自定义的`PrimitiveComponent`  

![BeginPlay](Img/MeshComponent.png)    

随后在detail面板中设置材质和网格体  
![BeginPlay](Img/MeshComponent-1.png)    

这里的材质并非什么都行，因为我用的是自定义的顶点工厂，可以在材质中自定义部分顶点着色器的行为。  
下图中，我使用custom节点让shaderinclude了我写的ush文件，这样就让这个ush文件中的代码起效，进而读取位置buffer  
![BeginPlay](Img/CustomVertexShader.png)    

不过现在有bug，有时候没有重新创建渲染状态，导致组件消失不见。这个问题还需要研究一下。
# 计算约束
距离约束统一保存在三个buffer当中，静态距离buffer、粒子A的ID的buffer、粒子B的ID的buffer。每次迭代都用计算着色器计算某一种约束，并将位置的变化量累积到buffer当中。  
这里的问题是，一个粒子可能被多个约束引用，所以写入时会有同步问题。这个问题暂时没研究透彻，现在是将变化量转为整形，然后用原子操作进行累加。    
另一种想到的方式是，我们可以在添加物体时，计算出每个约束对buffer的输出位置，这样可以在输出时，将同一个粒子的变化量输出到连续的空间，下一步用计算着色器做累加。  

# 刚性
对于刚性，一个简单的假设是材质的刚性除以迭代次数，但是，实际上迭代对结果的影响是非线性的。  
在[Position Based Dynamics](https://matthias-research.github.io/pages/publications/posBasedDyn.pdf)文中给出的一个思路是  

$$
k^`=1-(1-k)^{1/n_s}
$$  

而UE的chaos对其进行了修改，让这个相关于迭代次数的曲线变得更加陡峭，他的公式是   

$$
k^`=1-(1-1000^{k-1})^{1/n_s}
$$  


<div align=center><img src="./Img/Stiffness.png"><div>chaos的刚性系数随迭代次数变化更快，更陡峭</div></div>  

# 与CPU物体的交互
当前设想是分为GPU上运算的物体和CPU上运算的物体(引擎自己的物理计算)，但这样两者之间的交互就成了问题。   

现在是利用蓝图，将CPU物体添加到我的物理场景中 

![BeginPlay](Img/CPUObject-Begin.png "BeginPlay")    

每一次Tick都向插件同步当前的位置和速度，然后从插件中获取反馈

![Tick](Img/CPUObject-Tick.png)    

代码中，CPU物体添加到场景时会生成 `FCPUObjectProxy`，参考UE的 `SceneProxy`，每一次gamethread更新位置时，都会添加一条更新信息的Command，然后当renderthread中我的物理场景更新的时候，一起依据这些command更新renderthread里面的proxy的数值，保障线程的同步。    

```cpp
TMap<FCPUObjectProxy*, FUpdateCPUObjectTransformCommand> UpdatedCPUObjectTransformsRenderThread;
```

如果想要在预览窗口起效果，可以将`BeginPlay`相关逻辑移到`Construction Script`当中。

![Construction](Img/CPUObject-Construction.png)    

不过要记得将蓝图中的`Run Construction Script on Drag`取消，不然在拖拽时会不断创建物理代理。   

![Construction](Img/CPUObject-Drag.png)    


# 碰撞
碰撞还没做好，现在并没有真正的生成碰撞约束流，而是在其他约束计算之后，在一次dispatch中计算碰撞检测和反馈。  
每个线程对CPU物体的反馈(可能没有) 使用求前缀和的方式输出到Buffer中，然后cpu读取。  

```cpp
uint Thread = GroupThreadId.x;
GroupWriteOffsets[Thread] = bIsCollis;
GroupMemoryBarrierWithGroupSync();

// 线程组内部求前缀和
int OutBuffer = 0, InBuffer = 1;

[unroll]
for (uint Offset = 1; Offset < THREAD_COUNT; Offset = Offset << 1)
{

    OutBuffer = 1 - OutBuffer;
    InBuffer = 1 - InBuffer;
    if (Thread >= Offset)
    {
        GroupWriteOffsets[OutBuffer * THREAD_COUNT + Thread] = GroupWriteOffsets[InBuffer * THREAD_COUNT + Thread - Offset] + GroupWriteOffsets[InBuffer * THREAD_COUNT + Thread];
    }
    else
    {
        GroupWriteOffsets[OutBuffer * THREAD_COUNT + Thread] = GroupWriteOffsets[InBuffer * THREAD_COUNT + Thread];
    }

    GroupMemoryBarrierWithGroupSync();
}

// 线程组之间累加输出位置

if (Thread == 0)
{
    uint NumGroupFreeIDs = GroupWriteOffsets[(OutBuffer + 1) * THREAD_COUNT - 1];
    InterlockedAdd(FeedbackSizesBuffer[0], NumGroupFreeIDs, GroupWriteStart);
}

GroupMemoryBarrierWithGroupSync();

// 得到了线程组内和外的输出位置偏移，可以写了
if (bIsCollis)
{
    uint WriteOffset = GroupWriteStart + GroupWriteOffsets[OutBuffer * THREAD_COUNT + Thread] - bIsCollis;
    FeedbackBuffer[WriteOffset] = half4(NewColliderVelocity, 1.0);
}
```


这里一个尚未解决的问题是，如何对 "渲染线程写，game线程读"的方式进行同步。  

# 场景管理
我是打算将数据尽可能放到GPU上，不过现在欠缺些经验  
## 物理代理
我参考了UE中渲染使用的SceneProxy，写了这个PhysicsProxy  
理想上，不同的组件可以实现自己的物理代理，通过实现下面的函数来得到不同的表现。不过现在还在研究当中      
```cpp
virtual void GetDynamicPhysicsConstraints(FConstraintsBatch& OutBatch) const {};
virtual void GetCollisionInfo(FYQCollisionInfo& OutInfo) const {};
```

## 约束数据
```cpp
// 距离约束的Buffer
// 粒子A的ID
FRWBuffer DistanceConstraintsParticleAIDBuffer;
// 粒子B的ID
FRWBuffer DistanceConstraintsParticleBIDBuffer;
// 距离Buffer
FRWBuffer DistanceConstraintsDistanceBuffer;
```  
比较好理解，设计上里面的ID是封装的ID，里面的数据中几个bit存储这其他信息，比如是否是固定点，以此避免一个固定约束的pass。  
不过可能真的实现一个固定约束的pass反而比较好吧。  

## GPUObjectBuffer
这个是完全没有用上的设计，比较遗憾。  
本来是想走GPU Dirven，也就是用DispatchIndirect，所以想要维护一个Buffer来管理场景，但是还没想到合适的GPU Dirven思路。  
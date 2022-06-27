# 基于PBD的模拟的练习
在UE上练习物理模拟，尽可能不修改引擎，在插件中做全部，仍在施工中。     
# 模拟器类
使用了UE提供的WorldSubSystem实现了两个生命周期托管到引擎的单例类，PhyscisScene
和PhysicsSimulator，每一次tick时，PhyscisScene首先会整理各种Object的数据，遍历待加入到GPU场景的物体的列表，通过计算着色器将物体的数据写入到各种buffer中，比如位置buffer，同时根据需求产生约束。   
# SceneComponent
场景中用自定义的PrimitiveComponent来显示模拟的物体，自定义vertexfactory来fetch模拟出的顶点位置和法线，这可能带来大量的shader编译 (也是我开发时的困扰点，这很浪费时间)。   

# 布料
当前使用StaticMesh来生成布料粒子和约束，顶点色的R通道用来标定哪些是固定点。   

![基于网格生成布料](Img/ClothMeshVertexColorSample-0.png)  

![基于网格生成布料](Img/ClothMeshVertexColorSample-1.png)  

约束是使用IndexBuffer生成的，计算着色器的每个线程读取IndexBuffer中的3个值，然后生成3个距离约束。  
这样会有重复的约束生成，因为一个边可能被两个三角形共享。所以我做了一个GPU上的去重操作，大概就是结合排序和求前缀和   

* [3, 5, 3, 4, 1, 4, 9, 8, 6, 3, 5, 7, 8, 8, 4, 6, 4, 2, 5, 6] -- 0
* 排序，得到排序后的buffer，  
  [1, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 8, 8, 8, 9] -- 1  
* 对buffer-1 判断一个位置是否是连续相同数据(segment)的边界，其实就是判断左边的数据和自己是否相同   
  [1, 1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1] -- 2
* 对buffer-2 进行归约操作就能得到每个标为1的值的输出位置，进而将buffer压缩成unique的

也是参考了[tensorflow的做法](https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/kernels/unique_op_gpu.cu.h)。  
# CLtracEr
tracEr with OpenCL in C

Implemented with both single thread CPU and GPU, and connects to an OpenGL window to see real-time rendering 

Supports diffuse, glossy, and transmissive shaders

Currently only supports spheres, and does not have BVH support - does direct traversal with zero order 

Jittering uses Halton Sampling to get deterministic "pseudorandom" stratification that helps with aliasing to get uniform sampling

Code sucks


# Improvements over original tracEr:
Massively simplified and removes all objects and polymorphism for a streamlined calculation

Transmissive Shaders (GPU transmission is messed up I will eventually get to debugging it)

Interactive scenes, the basic scene runs at 60fps @ 10 samples per frame on my M1 Macbook Air (600 samples/sec) at 600x400 resolution. 
  <sub>(for fun, assuming ~ 5 bounces per sample we get 720 million "light casts" per second - there's a lot of extra operations in a light cast and I really don't know how to calculate this, but the M1 gpu with 7 gpu cores has 2.3 tflops theoretical performance (2.6 * 7/8), so the estimated number of floating point operations per light cast is around 3,200 - this is a high estimate, given that it likely is not at max performance + there is some latency in sending data from cpu -> gpu and vice versa, both are blocking operations, and both happen on every single frame to update the data. Useless yes but pretty cool to see how much math is actually needed to do a single light bounce. thank you for coming to my ted talk LOL)
   (for the not so fun part, there is no BVH implemented yet so increasing the number of objects will be followed by a linear increase to the runtime and hit on the performance, so this 3,200 only works for the default scene)</sub>

# Future Plans:
BVH

Triangle Intersections (this one is quick)

Image textures

Photon Mapping 

Custom Scene Loading instead of one single scene

# Scenes
(Rendered using CPU ~5000 samples, took a couple mins)

![Alt text](https://cdn.discordapp.com/attachments/680818011548024835/903439437151760394/uwu.png) 


(Rendered using GPU, took a couple seconds 😎)

![Alt text](https://cdn.discordapp.com/attachments/470588222779424768/988276258418876456/Screen_Shot_2022-06-19_at_10.56.54_PM.png) 

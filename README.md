# CLtracEr
tracEr with OpenCL in C

Implemented with both single thread CPU and GPU, and connects to an OpenGL window to see real-time rendering (this destroys perf though)

Supports diffuse, glossy, and transmissive (cpu only) shaders

Currently only supports spheres, and does not have BVH support


# Improvements over original tracEr:
Massively simplified and removes all objects and polymorphism

Transmissive Shaders (only on cpu side though)

GPU support with OpenCL

# Future Plans:
BVH

Triangle Intersections (this one is quick)

Image textures

Photon Mapping 

Interactive Window (moving around)

Custom Scene Loading instead of one single scene

# Scenes
![Alt text](https://cdn.discordapp.com/attachments/680818011548024835/903439437151760394/uwu.png)

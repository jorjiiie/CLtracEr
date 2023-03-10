# CLtracEr
* tracEr with OpenCL in C

* Implemented with both single thread CPU and GPU, and connects to an OpenGL window to see real-time rendering 

* GPU enables smooth interactions, capable running at over 60 fps at reasonable quality


* Supports diffuse, glossy, and transmissive shaders

# Improvements over original tracEr:
* Massively simplified and removes all objects and polymorphism for a streamlined calculation, increasing speed

* Transmissive Shaders

* GPU support and interactivity



# Future Plans:
* BVH/Other acceleration structures

* Triangles 

* Image textures & texture mapping

* Photon Mapping as further acceleration

* Custom Scene Loading instead of one single hardcoded scene

# Scenes
(Rendered using CPU ~5000 samples, took a couple mins on an M1)

![Alt text](https://cdn.discordapp.com/attachments/680818011548024835/903439437151760394/uwu.png) 


(Rendered using GPU, took ~10 seconds on M1 GPU)

![Alt text](https://cdn.discordapp.com/attachments/470588222779424768/988276258418876456/Screen_Shot_2022-06-19_at_10.56.54_PM.png) 

# Nerd Details

* Jittering uses Halton Sampling to get deterministic "pseudorandom" stratification that helps with aliasing to get uniform sampling

* Utilizes a biased light calculation (I believe) and for all real purposes this project is doomed, see my new raytracer (to be released summer 2023) for the promised improvements in code and features.

* Fast RNG using PCG32 on the GPU

* Transmissive shaders on the GPU are (clearly) broken
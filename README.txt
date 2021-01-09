Ray-tracing Renderer


Overview:
	A basic ray-tracing rendering engine.

	Uses one-directional path tracing, which approximates the rendering equation at surface points using Monte Carlo integration.
	The Cook-Torrance bidirectional reflection distribution function, based on microfacet surface theory, is used, with the
	GGX microfacet normal distribution.
	Monte Carlo integration is done via importance sampling, according to the GGX distribution.
	Light transmission, i.e. surface transparency, is not currently supported.

	Capable of rendering arbitrary polygon meshes.

	The renderer is CPU-optimised, with no GPU acceleration support.


Requirements:
	- C++ 17.
	- CPU with AVX2.
	- Threading Building Blocks (TBB), if on a Linux system. Required for std::execution.


Building:
	The project is built with CMake. The executable is built into a "bin" directory in the project root.
	Unless debugging, please build the project in release mode, as debug mode is too slow for any real renders.

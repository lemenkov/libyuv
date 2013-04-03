psnr: psnr.cc ssim.cc psnr_main.cc
	$(CXX) -msse2 -O3 -fopenmp -static -o psnr psnr.cc ssim.cc psnr_main.cc -Wl,--strip-all

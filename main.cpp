/*******************************************************************
*   main.cpp
*   ULATCH
*
*	Author: Kareem Omar
*	kareem.omar@uah.edu
*	https://github.com/komrad36
*
*	Last updated Sep 12, 2016
*******************************************************************/
//
// Fastest CPU implementation of an UPRIGHT (no rotation)
// LATCH 512-bit binary feature descriptor
// as described in the 2015 paper by
// Levi and Hassner:
//
// "LATCH: Learned Arrangements of Three Patch Codes"
// http://arxiv.org/abs/1501.03719
//
// See also the ECCV 2016 Descriptor Workshop paper, of which I am a coauthor:
//
// "The CUDA LATCH Binary Descriptor"
// http://arxiv.org/abs/1609.03986
//
// And the original LATCH project's website:
// http://www.openu.ac.il/home/hassner/projects/LATCH/
//
// See my GitHub for this CUDA version, which is extremely fast.
//
// Note once again that this is an UPRIGHT LATCH, a.k.a. ULATCH.
// A fast rotation- and scale-invariant version is
// also available on my GitHub.
//
// My implementation uses multithreading, SSE2/3/4/4.1, AVX, AVX2, and 
// many many careful optimizations to implement the
// algorithm as described in the paper, but at great speed.
// This implementation outperforms the reference implementation by 800%
// single-threaded or 3200% multi-threaded (!) while exactly matching
// the reference implementation's output and capabilities in upright mode.
//
// All functionality is contained in the file ULATCH.h. 'main.cpp'
// is simply a sample test harness with example usage and
// performance testing.
//

#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

#include "ULATCH.h"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN

using namespace std::chrono;

int main() {
	// ------------- Configuration ------------
	constexpr int warmups = 30;
	constexpr int runs = 100;
	constexpr int numkps = 5000;
	constexpr bool multithread = true;
	constexpr char name[] = "test.jpg";
	// --------------------------------


	// ------------- Image Read ------------
	cv::Mat image = cv::imread(name, CV_LOAD_IMAGE_GRAYSCALE);
	if (!image.data) {
		std::cerr << "ERROR: failed to open image. Aborting." << std::endl;
		return EXIT_FAILURE;
	}
	// --------------------------------


	// ------------- Detection ------------
	std::cout << std::endl << "Detecting..." << std::endl;
	cv::Ptr<cv::ORB> orb = cv::ORB::create(numkps, 1.2f, 8, 31, 0, 2, cv::ORB::HARRIS_SCORE, 31, 20);
	std::vector<cv::KeyPoint> keypoints;
	orb->detect(image, keypoints);
	// --------------------------------


	// ------------- ULATCH ------------
	uint64_t* desc = new uint64_t[8 * keypoints.size()];
	std::vector<KeyPoint> kps;
	for (auto&& kp : keypoints) kps.emplace_back(kp.pt.x, kp.pt.y);
	std::cout << "Warming up..." << std::endl;
	for (int i = 0; i < warmups; ++i) ULATCH<multithread>(image.data, image.cols, image.rows, static_cast<int>(image.step), kps, desc);
	std::cout << "Testing..." << std::endl;
	high_resolution_clock::time_point start = high_resolution_clock::now();
	for (int i = 0; i < runs; ++i) ULATCH<multithread>(image.data, image.cols, image.rows, static_cast<int>(image.step), kps, desc);
	high_resolution_clock::time_point end = high_resolution_clock::now();
	// --------------------------------

	std::cout << std::endl << "ULATCH took " << static_cast<double>((end - start).count()) * 1e-3 / (static_cast<double>(runs) * static_cast<double>(kps.size())) << " us per desc over " << kps.size() << " desc" << (kps.size() == 1 ? "." : "s.") << std::endl << std::endl;

	long long total = 0;
	for (size_t i = 0; i < 8 * kps.size(); ++i) total += desc[i];
	std::cout << "Checksum: " << std::hex << total << std::endl << std::endl;
}

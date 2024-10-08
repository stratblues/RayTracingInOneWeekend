// RayTracer.cpp : Defines the entry point for the application.
//
#include "rtweekend.h"

#include "camera.h"
#include "hittable.h"
#include "hittable_list.h"
#include "sphere.h"



/*
**To run program in debug not production build**

cmake -B build
cmake --build build

build\Debug\RayTracer.exe > image.ppm

*/




int main()
{
	// World
	hittable_list world;

	world.add(make_shared<sphere>(point3(0, 0, -1), 0.5));
	world.add(make_shared<sphere>(point3(0, -100.5, -1), 100));

	camera cam;

	cam.aspect_ratio = 16.0 / 9.0;
	cam.image_width = 400;

	cam.render(world);
	
}

#ifndef CAMERA_H
#define CAMERA_H

#include "RTWeekend.h"    // For constants like infinity
#include "vec3.h"         // For vec3 and point3 types
#include "ray.h"          // For the ray class
#include "hittable.h"     // For hittable and hit_record
#include "color.h"        // For the color class
#include "interval.h"     // For the interval class
#include "material.h"

#include <thread>
#include <vector>
#include <mutex>

class camera
{
public:
    /* Public Camera Parameters Here */
    // Image

    double aspect_ratio = 16.0 / 9.0;
    int image_width = 400;
    int samples_per_pixel = 10;
    int max_depth = 10;

    void render(const hittable& world)
    {
        initialize();

        // Create an image buffer to store the computed colors
        std::vector<color> image(image_width * image_height);

        // Determine the number of hardware threads available
        unsigned int thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0) thread_count = 4; // Fallback to 4 threads if unable to detect

        // Divide the image into vertical slices for each thread
        std::vector<std::thread> threads(thread_count);

        int rows_per_thread = image_height / thread_count;
        int extra_rows = image_height % thread_count;

        // Launch threads
        int start_row = 0;
        for (unsigned int t = 0; t < thread_count; ++t)
        {
            int end_row = start_row + rows_per_thread;
            if (t < extra_rows) ++end_row;

            threads[t] = std::thread(&camera::render_section, this, start_row, end_row, &world, &image);

            start_row = end_row;
        }

        // Wait for all threads to complete
        for (auto& thread : threads)
        {
            thread.join();
        }

        // Output the final image
        std::cout << "P3\n" << image_width << ' ' << image_height << "\n255\n";

        for (int j = 0; j < image_height; ++j)
        {
            for (int i = 0; i < image_width; ++i)
            {
                color pixel_color = image[j * image_width + i];
                write_color(std::cout, pixel_color);
            }
        }

        std::clog << "\rDone               \n";
    }

private:
    /* Private Camera Variables Here */
    int image_height;
    double pixel_samples_scale;
    point3 center;
    point3 pixel00_loc;
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;

    void initialize()
    {
        // Calculate the image height, and ensure that it's at least 1.
        image_height = int(image_width / aspect_ratio);
        image_height = (image_height < 1) ? 1 : image_height;

        pixel_samples_scale = 1.0 / samples_per_pixel;
        center = point3(0, 0, 0);

        // Viewport dimensions
        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (double(image_width) / image_height);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = vec3(viewport_width, 0, 0);
        auto viewport_v = vec3(0, -viewport_height, 0);

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_width;
        pixel_delta_v = viewport_v / image_height;

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = center
            - vec3(0, 0, focal_length) - viewport_u / 2 - viewport_v / 2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }

    // Antialiasing
    ray get_ray(int i, int j) const
    {
        // Construct a camera ray originating from the origin and directed at randomly sampled
        // point around the pixel location i,j.

        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
            + ((i + offset.x()) * pixel_delta_u)
            + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }

    vec3 sample_square() const
    {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }

    color ray_color(const ray& r, int depth, const hittable& world) const
    {
        if (depth <= 0)
        {
            return color(0, 0, 0);
        }
        hit_record rec;
        if (world.hit(r, interval(0.001, infinity), rec))
        {
            ray scattered;
            color attenuation;
            if (rec.mat->scatter(r, rec, attenuation, scattered))
            {
                return attenuation * ray_color(scattered, depth - 1, world);
            }
            return color(0, 0, 0);
        }
        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5 * (unit_direction.y() + 1.0);
        return (1.0 - a) * color(1.0, 1.0, 1.0) + a * color(0.5, 0.7, 1.0);
    }

    void render_section(int start_row, int end_row, const hittable& world, std::vector<color>& image)
    {
        for (int j = start_row; j < end_row; ++j)
        {
            // Update progress
            {
                static std::mutex mutex;
                std::lock_guard<std::mutex> lock(mutex);
                std::clog << "\rScanlines remaining: " << (image_height - j) << ' ' << std::flush;
            }

            for (int i = 0; i < image_width; ++i)
            {
                color pixel_color(0, 0, 0);
                for (int sample = 0; sample < samples_per_pixel; ++sample)
                {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                pixel_color *= pixel_samples_scale;

                // Store the computed color in the image buffer
                image[j * image_width + i] = pixel_color;
            }
        }
    }
};

#endif

/*
 * Copyright © 2008 Ben Smith
 * Copyright © 2010-2011 Linaro Limited
 *
 * This file is part of the glmark2 OpenGL (ES) 2.0 benchmark.
 *
 * glmark2 is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.
 *
 * glmark2 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * glmark2.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  Ben Smith (original glmark benchmark)
 *  Alexandros Frantzis (glmark2)
 */
#include "oglsdl.h"
#include "scene.h"
#include "benchmark.h"
#include "options.h"
#include "log.h"

#include <iostream>

#if USE_GL
#include "screen-sdl-gl.h"
#elif USE_GLESv2
#include "screen-sdl-glesv2.h"
#endif

using std::vector;
using std::map;
using std::string;

bool should_keep_running()
{
    bool running = true;
    SDL_Event event;

    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_ESCAPE)
                    running = false;
                break;
        }
    }

    return running;
}

void
add_default_benchmarks(vector<Benchmark *> &benchmarks)
{
    vector<Benchmark::OptionPair> opts;

    opts.clear();
    opts.push_back(Benchmark::OptionPair("use-vbo", "false"));
    benchmarks.push_back(new Benchmark("build", opts));

    opts.clear();
    opts.push_back(Benchmark::OptionPair("use-vbo", "true"));
    benchmarks.push_back(new Benchmark("build", opts));

    opts.clear();
    opts.push_back(Benchmark::OptionPair("texture-filter", "nearest"));
    benchmarks.push_back(new Benchmark("texture", opts));

    opts.clear();
    opts.push_back(Benchmark::OptionPair("texture-filter", "linear"));
    benchmarks.push_back(new Benchmark("texture", opts));

    opts.clear();
    opts.push_back(Benchmark::OptionPair("texture-filter", "mipmap"));
    benchmarks.push_back(new Benchmark("texture", opts));

    opts.clear();
    opts.push_back(Benchmark::OptionPair("shading", "gouraud"));
    benchmarks.push_back(new Benchmark("shading", opts));

    opts.clear();
    opts.push_back(Benchmark::OptionPair("shading", "phong"));
    benchmarks.push_back(new Benchmark("shading", opts));
}

static void
list_scenes()
{
    const map<string, Scene *> &scenes = Benchmark::scenes();

    for (map<string, Scene *>::const_iterator scene_iter = scenes.begin();
         scene_iter != scenes.end();
         scene_iter++)
    {
        Scene *scene = scene_iter->second;
        Log::info("[Scene] %s\n", scene->name().c_str());

        const map<string, Scene::Option> &options = scene->options();

        for (map<string, Scene::Option>::const_iterator opt_iter = options.begin();
             opt_iter != options.end();
             opt_iter++)
        {
            const Scene::Option &opt = opt_iter->second;
            Log::info("  [Option] %s\n"
                      "    Description  : %s\n"
                      "    Default Value: %s\n",
                      opt.name.c_str(), 
                      opt.description.c_str(),
                      opt.default_value.c_str());
        }
    }
}

int main(int argc, char *argv[])
{
    unsigned score = 0;

    if (!Options::parse_args(argc, argv))
        return 1;

    if (Options::show_help) {
        Options::print_help();
        return 0;
    }

    // Create the screen
#if USE_GL
    ScreenSDLGL screen(800, 600, 24, 0);
#elif USE_GLESv2
    ScreenSDLGLESv2 screen(800, 600, 24, 0);
#endif

    if (!screen.mInitSuccess) {
        printf("Error: %s: Could not initialize screen\n", __FUNCTION__);
        return 1;
    }

    // Register the scenes, so they can be looked-up by name
    Benchmark::register_scene(*new SceneBuild(screen));
    Benchmark::register_scene(*new SceneTexture(screen));
    Benchmark::register_scene(*new SceneShading(screen));

    if (Options::list_scenes) {
        list_scenes();
        return 0;
    }

    // Add the benchmarks to run
    vector<Benchmark *> benchmarks;

    add_default_benchmarks(benchmarks);

    printf("=======================================================\n");
    printf("    glmark2 %s\n", GLMARK_VERSION);
    printf("=======================================================\n");
    screen.print_info();
    printf("=======================================================\n");

    // Run the benchmarks
    for (vector<Benchmark *>::iterator bench_iter = benchmarks.begin();
         bench_iter != benchmarks.end();
         bench_iter++)
    {
        bool keep_running = true;
        Benchmark *bench = *bench_iter;
        Scene &scene = bench->setup_scene();
        std::cout << scene.info_string() << std::flush;

        while (scene.is_running() &&
               (keep_running = should_keep_running()))
        {
            screen.clear();

            scene.draw();
            scene.update();

            screen.update();
        }

        std::cout << " FPS: " << scene.average_fps() << std::endl;
        score += scene.average_fps();

        bench->teardown_scene();

        if (!keep_running)
            break;
    }

    printf("=======================================================\n");
    printf("                                  glmark2 Score: %u \n", score);
    printf("=======================================================\n");

    return 0;
}

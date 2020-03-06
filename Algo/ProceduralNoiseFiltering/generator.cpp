#include "histogram.h"
#include "gaussian_transfer.h"
#include "color_map.h"
#include "pnf.h"
#include <unistd.h>
#include <cstdio>
#include <getopt.h>
#include <ASTex/easy_io.h>
#include <ASTex/rpn_utils.h>
#include <ASTex/exr_io.h>

using namespace ASTex;
using Color = Color_map<double>::Color;
using Vec2 = Pnf<double>::Vec2;

static void usage(const char *msg = nullptr, ...) {
    if (msg) {
        va_list args;
        va_start(args, msg);
        fprintf(stderr, "generator: ");
        vfprintf(stderr, msg, args);
        fprintf(stderr, "\n");
    }
    fprintf(stderr, R"(usage: generator <command> [options] <filenames...>
    commands: histo, gaussianize, getDegaussTrans, prefilter, PSD, noisePSD,
              noiseCm, noiseCmGroundTruth, noiseCmNaiveFiltering, noiseCmHeitzFiltering, noiseCmOurFiltering


    histo options:
        -o                  name of the output file (default histo).
        --outfile           name of the output file (default histo).
        -n                  nb_bins (default 256).
        --nb_bins           nb_bins (default 256).
        -y                  range max of y of the histogram (default 1.0).
        --y_range_max       range max of y of the histogram (default 1.0).
        -g                  to get the gray histogram (default RGB).
        --gray              to get the gray histogram (default RGB).
        --16                to load a 16 bits image (default load 8 bits image).

    gaussianize options:
        -o                  name of the output file (default out.png).
        --outfile           name of the output file (default out.png).
        -g                  to transform a gray image (default transform RGB image).
        --gray              to transform a gray image (default transform RGB image).
        --16                to load and save a 16 bits image (default load and save 8 bits image).

    get_degauss_trans options:
        -o                  name of the output file (default lut.png).
        --outfile           name of the output file (default lut.png).
        -s                  size of the lookUpTable (lut) (default 256).
        --size              size of the lookUpTable (lut) (default 256).
        -g                  to get the lut (= degaussianize transformation) of a gray image (default transform RGB image).
        --gray              to get the lut (= degaussianize transformation) of a gray image (default transform RGB image).
        --16                to load and save a 16 bits image (default load and save 8 bits image).

    prefilter options:
        -o                  name of the output file (default color_map_flitered.png).
        --outfile           name of the output file (default color_map_flitered.png).
        -w                  size of the width of the output image (default 256).
        --width             size of the width of the output image (default 256).
        -h                  size of the height of the output image (default 256).
        --height            size of the height of the output image (default 256).
        -n                  number of sampling to do the numerical integration (default 200).
        --lut               (optional) name of the lut used to do the prefiltering (default lut.png)
        --16                to load a 16 bits lut (default load 8 bits lut).

    PSD options:
        -o                  name of the output file (default psd).
        --outfile           name of the output file (default psd).
        --exr               to get the psd in exr format (default format png).
        --phases            (optional) name of the phases (default phases.png).

    noisePSD options:
        -o                  name of the output file (default noise.png).
        --outfile           name of the output file (default noise.png).
        -m                  mean of the noise.
        --mu                mean of the noise.
        -s                  std deviation of the noise.
        --sigma             std deviation of the noise.
        --exr               load a exr format of PSD.

    noiseCm options:
        -o                  name of the output file (default noise_color_mapped.png).
        --outfile           name of the output file (default noise_color_mapped.png).
        -w                  width of the output image (default 512).
        --width             width of the output image (default 512).
        -h                  height of the output image (default 512).
        --height            height of the output image (default 512).
        -z                  level of zoom out (default 1).
        --zoomOut           level of zoom out (default 1).
        -e                  name of the example noise.
        --example           name of the example noise.
        --noColorMap        to not use the color map.
        --numberOfCosines   number of cosines of the noise (default 32).
        --frequenceMin      minimal frequence of the noise (default 10).
        --frequenceMax      maximal frequence of the noise (default 20).
        --16                load the example and save the noise with 16bits.

    noiseCmGroundTruth, noiseCmNaiveFiltering and noiseCmHeitzFiltering  options:
        -o                  name of the output file (default noise_color_mapped.png).
        --outfile           name of the output file (default noise_color_mapped.png).
        -w                  width of the output image (default 512).
        --width             width of the output image (default 512).
        -h                  height of the output image (default 512).
        --height            height of the output image (default 512).
        -z                  level of zoom out (default 1).
        --zoomOut           level of zoom out (default 1).
        -e                  name of the example noise.
        --example           name of the example noise.
        --noColorMap        to not use the color map.
        --nbSampling        the (sqrt) number of sampling (default 10).
        --numberOfCosines   number of cosines of the noise (default 32).
        --frequenceMin      minimal frequence of the noise (default 10).
        --frequenceMax      maximal frequence of the noise (default 20).
        --16                load the example and save the noise with 16bits.


    noiseCmOurFiltering options:
        -o                  name of the output file (default noise_color_mapped.png).
        --outfile           name of the output file (default noise_color_mapped.png).
        -w                  width of the output image (default 512).
        --width             width of the output image (default 512).
        -h                  height of the output image (default 512).
        --height            height of the output image (default 512).
        -z                  level of zoom out (default 1).
        --zoomOut           level of zoom out (default 1).
        --16                load the example and save the noise with 16bits.


)");
    exit(EXIT_FAILURE);
}

int histo(int argc, char **argv)
{
    if(argc < 2) usage(argv[0]);

    bool gray = false;
    bool _16bits = false;
    int nb_bins = 256;
    std::string filename("histo");
    float ymax = 1.0;

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",     required_argument,  nullptr,    'o'},
        {"nb_bins",     required_argument,  nullptr,    'n'},
        {"y_range_max", required_argument,  nullptr,    'y'},
        {"gray" ,       no_argument,        nullptr,    'g' },
        {"16",          no_argument,        nullptr,    16},
        {0,             0,                  0,          0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:n:y:g",long_options,&option_index)) != -1) {
        switch (opt) {
            case 16:
                _16bits = true;
                break;
            case 'g':
                gray = true;
                break;
            case 'n':
                nb_bins = std::atoi(optarg);
                break;
            case 'o':
                filename = optarg;
                break;
            case 'y':
                ymax = float(std::atof(optarg));
                break;
            default:
                usage(argv[0]);

        }

    }

    if(gray){
        Histogram<ImageGrayd> h;
        ImageGrayd img;
        if(_16bits)
            IO::loadu16_in_01(img, argv[optind]);
        else
            IO::loadu8_in_01(img, argv[optind]);
        h.computeHisto(img, nb_bins);
        h.exportHisto(filename, ymax);
    }
    else {
        Histogram<ImageRGBd> h;
        ImageRGBd img;
        if(_16bits)
            IO::loadu16_in_01(img, argv[optind]);
        else
            IO::loadu8_in_01(img, argv[optind]);
        h.computeHisto(img, nb_bins);
        h.exportHisto(filename, ymax);
    }

    return EXIT_SUCCESS;
}

int gaussianize(int argc, char **argv)
{
    if(argc < 2) usage(argv[0]);

    bool gray = false;
    bool _16bits = false;
    std::string filename("out.png");

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",     required_argument,  nullptr,    'o'},
        {"gray" ,       no_argument,        nullptr,    'g' },
        {"16",          no_argument,        nullptr,    16},
        {0,             0,                  0,          0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:g",long_options,&option_index)) != -1) {
        switch (opt) {
            case 16:
                _16bits = true;
                break;
            case 'g':
                gray = true;
                break;
            case 'o':
                filename = optarg;
                break;
            default:
                usage(argv[0]);

        }

    }

    if(gray){
        ImageGrayf img;
        if(_16bits)
            IO::loadu16_in_01(img, argv[optind]);
        else
            IO::loadu8_in_01(img, argv[optind]);

        ImageGrayf imgT(img.width(), img.height());
        Gaussian_transfer::ComputeTinput(img,imgT);

        if(_16bits)
            IO::save01_in_u16(imgT, filename);
        else
            IO::save01_in_u8(imgT, filename);
    }
    else {
        ImageRGBf img;
        if(_16bits)
            IO::loadu16_in_01(img, argv[optind]);
        else
            IO::loadu8_in_01(img, argv[optind]);

        ImageRGBf imgT(img.width(), img.height());
        Gaussian_transfer::ComputeTinput(img,imgT);

        if(_16bits)
            IO::save01_in_u16(imgT, filename);
        else
            IO::save01_in_u8(imgT, filename);
    }

    return EXIT_SUCCESS;

}

int getDegaussTrans(int argc, char **argv)
{
    if(argc < 2) usage(argv[0]);

    bool gray = false;
    bool _16bits = false;
    std::string filename("lut.png");
    int s = 256;

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",     required_argument,  nullptr,    'o'},
        {"gray" ,       no_argument,        nullptr,    'g'},
        {"16",          no_argument,        nullptr,    16 },
        {"size",        required_argument,  nullptr,    's'},
        {0,             0,                  0,          0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:gs:",long_options,&option_index)) != -1) {
        switch (opt) {
            case 16:
                _16bits = true;
                break;
            case 'g':
                gray = true;
                break;
            case 'o':
                filename = optarg;
                break;
            case 's':
                s = std::atoi(optarg);
                break;
            default:
                usage(argv[0]);

        }

    }

    if(gray){
        ImageGrayf img;
        if(_16bits)
            IO::loadu16_in_01(img, argv[optind]);
        else
            IO::loadu8_in_01(img, argv[optind]);

        ImageGrayf ilut(s, 1);
        Gaussian_transfer::ComputeinvT(img,ilut);

        if(_16bits)
            IO::save01_in_u16(ilut, filename);
        else
            IO::save01_in_u8(ilut, filename);
    }
    else {
        ImageRGBf img;
        if(_16bits)
            IO::loadu16_in_01(img, argv[optind]);
        else
            IO::loadu8_in_01(img, argv[optind]);

        ImageRGBf ilut(s, 1);
        Gaussian_transfer::ComputeinvT(img,ilut);

        if(_16bits)
            IO::save01_in_u16(ilut, filename);
        else
            IO::save01_in_u8(ilut, filename);
    }

    return EXIT_SUCCESS;

}

int prefilter(int argc, char **argv)
{
    bool lut = false;
    bool _16bits = false;
    std::string filename("color_map_prefiltered.png");
    std::string lutname("lut.png");
    int w(256), h(256);
    int nb(200);
    double sigma_max(1./6.);

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",     required_argument,  nullptr,    'o'},
        {"16",          no_argument,        nullptr,    16 },
        {"width",       required_argument,  nullptr,    'w'},
        {"height",      required_argument,  nullptr,    'h'},
        {"sigmaMax",    required_argument,  nullptr,    's'},
        {"lut",         optional_argument,  nullptr,    1  },
        {0,             0,                  0,          0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:w:h:n:s:",long_options,&option_index)) != -1) {
        switch (opt) {
            case 1:
                lut = true;
                if(optarg)
                    lutname = optarg;
                break;
            case 16:
                _16bits = true;
                break;
            case 'o':
                filename = optarg;
                break;
            case 'w':
                w = std::atoi(optarg);
                break;
            case 'h':
                h = std::atoi(optarg);
                break;
            case 'n':
                nb = std::atoi(optarg);
                break;
            case 's':
                sigma_max = std::atof(optarg);
                break;
            default:
                usage(argv[0]);

        }

    }

    Color_map<double> cm;
//  palette a
//    cm.add_color(0,Color(1,1,0));
//    cm.add_color(40,Color(1,0,0));
//    cm.add_color(59,Color(0,0,0));
//    cm.add_color(60,Color(1,1,1));
//    cm.add_color(100,Color(1,1,1));

//  palette b
    cm.add_color(0, Color(0,0,0));
    cm.add_color(4, Color(0,0,0));
    cm.add_color(5, Color(1,1,1));
    cm.add_color(7, Color(1,1,1));
    cm.add_color(9, Color(199./255., 139./255., 105./255.));
    cm.add_color(10, Color(199./255., 139./255., 105./255.));

//  palette c
//    cm.add_color(0, Color(0., 0., 1.));
//    cm.add_color(1, Color(1., 0., 0.));

//  palette d
//    cm.add_color(0,Color(0,0,1));
//    cm.add_color(1,Color(0,1,0));
//    cm.add_color(2,Color(1,0,0));

    if(lut){
        ImageGrayf ilut;
        if(_16bits)
            IO::loadu16_in_01(ilut,lutname);
        else
            IO::loadu8_in_01(ilut,lutname);

        cm.set_degauss(ilut);
    }

    cm.filter(w, h, nb, sigma_max);
    ImageRGBd filtered = cm.get_filtered();

    IO::save01_in_u8(filtered, filename);

    std::cout << "sigma max = " << sigma_max << std::endl;

    return EXIT_SUCCESS;

}

int PSD(int argc, char **argv)
{
    if(argc < 2) usage(argv[0]);

    bool exr = false;
    bool p = false;
    std::string psdname("psd");
    std::string phasesname("phases.png");

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",     required_argument,  nullptr,    'o'},
        {"exr",         no_argument,        nullptr,    'e' },
        {"phases",      optional_argument,  nullptr,    'p' },
        {0,             0,                  0,          0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:",long_options,&option_index)) != -1) {
        switch (opt) {
            case 'o':
                psdname = optarg;
                break;
            case 'e':
                exr = true;
                break;
            case 'p':
                p = true;
                if(optarg)
                    phasesname = optarg;
                break;
            default:
                usage(argv[0]);

        }

    }

    ImageGrayd example;
    IO::loadu8_in_01(example, argv[optind]);

    ImageSpectrald psd, phase;
    Fourier::fftForwardModulusAndPhase(example, psd, phase);

    if(exr){
        IO::EXR::save(psd, psdname + ".exr");
    }
    else {
        IO::save01_in_u8(psd, psdname +".png");
    }

    if(p)
        IO::save_phase(phase, phasesname);

    return EXIT_SUCCESS;

}

int noisePSD(int argc, char **argv)
{
    if(argc < 2) usage(argv[0]);

    bool exr = false;
    bool mu = false;
    bool std_dev = false;
    double m;
    double s;
    std::string filename("noise.png");

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",     required_argument,  nullptr,    'o'},
        {"mu" ,         required_argument,  nullptr,    'g'},
        {"sigma",       required_argument,  nullptr,    's'},
        {"exr",         no_argument,        nullptr,    'e' },
        {0,             0,                  0,          0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:m:s:",long_options,&option_index)) != -1) {
        switch (opt) {
            case 'o':
                filename = optarg;
                break;
            case 'm':
                mu = true;
                m = std::atof(optarg);
                break;
            case 's':
                std_dev = true;
                s = std::atof(optarg);
                break;
            case 'e':
                exr = true;
                break;
            default:
                usage(argv[0]);

        }

    }

    ImageSpectrald psd;

    if(exr)
        IO::EXR::load(psd, argv[optind]);
    else
        IO::loadu8_in_01(psd, argv[optind]);

    ImageGrayd noise(psd.width(),psd.height());

    ImageSpectrald phase;
    rpn_scalar(psd, phase, noise);

    double mean = getMean(noise);
    double sigma = getStDev(noise);

    noise.parallel_for_all_pixels([&] (ImageGrayd::PixelType &pix)
    {
        if(std_dev)
            pix *= s * 1.0/sigma;
        if(mu)
            pix += m - mean;

        pix = clamp_scalar(pix, 0., 1.);
    });

    IO::save01_in_u8(noise, filename);

    std::cout << "mu = " << getMean(noise) << std::endl;
    std::cout << "std dev = " << getStDev(noise) << std::endl;

    return EXIT_SUCCESS;

}

int noiseCm(int argc, char **argv)
{
    if(argc < 3) usage(argv[0]);

    std::string filename("noise_color_mapped.png");
    std::string inputname;
    int w(512),h(512);
    double z(1);

    int number_of_cosines(32);
    double fr_min(10), fr_max(20);

    bool cm(true);
    bool example(false);
    bool _16bits(false);

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",         required_argument,  nullptr,    'o'},
        {"width" ,          required_argument,  nullptr,    'w'},
        {"height",          required_argument,  nullptr,    'h'},
        {"zoomOut",         required_argument,  nullptr,    'z'},
        {"example",         required_argument,  nullptr,    'e'},
        {"noColorMap",      no_argument,        nullptr,     1},
        {"numberOfCosines", required_argument,  nullptr,     2},
        {"frequenceMin",    required_argument,  nullptr,     3},
        {"frequenceMax",    required_argument,  nullptr,     4},
        {"16",              no_argument,        nullptr,     16 },
        {0,                 0,                  0,           0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:w:h:z:e:",long_options,&option_index)) != -1) {
        switch (opt) {
            case 1:
                cm = false;
                break;
            case 2:
                number_of_cosines = std::atoi(optarg);
                break;
            case 3:
                fr_min = std::atof(optarg);
                break;
            case 4:
                fr_max = std::atof(optarg);
                break;
            case 16:
                _16bits= true;
                break;
            case 'o':
                filename = optarg;
                break;
            case 'w':
                w = std::atoi(optarg);
                break;
            case 'h':
                h = std::atoi(optarg);
                break;
            case 'z':
                z = std::atof(optarg);
                break;
            case 'e':
                example = true;
                inputname = optarg;
                break;
            default:
                usage(argv[0]);

        }

    }

    ImageRGBd prefiltered;
    IO::loadu8_in_01(prefiltered, argv[optind]);
    double sigma_max = std::atof(argv[optind+1]);

    Color_map<double> color_map;
    color_map.set_filtered(prefiltered, sigma_max);

    Vec2 im_size(w,h);
    std::chrono::time_point<std::chrono::system_clock> start_chrono;
    std::chrono::duration<double> elapsed_seconds;
    if(!example)
    {
        Noise<double> n(number_of_cosines, fr_min, fr_max);
        Vec2 w_size(z,z);
        if(cm){
            start_chrono= std::chrono::system_clock::now();

            ImageRGBd img = Pnf<double>::compute_unfiltered_IMG(w_size, im_size, n, color_map);

            elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
            std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

            IO::save01_in_u8(img, filename);
        }
        else {
            start_chrono= std::chrono::system_clock::now();

            ImageGrayd img = Pnf<double>::compute_noise_unfiltered(w_size, im_size, n);

            elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
            std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;
            IO::save01_in_u8(img, filename);
        }
    }
    else {
        ImageGrayd noise_example;
        if(_16bits)
            IO::loadu16_in_01(noise_example, inputname);
        else
            IO::loadu8_in_01(noise_example, inputname);

        TextureNoise<double> n;
        n.setNoise(noise_example);
        Vec2 w_size(noise_example.width()*z, noise_example.height()*z);

        if(cm){
            start_chrono= std::chrono::system_clock::now();

            ImageRGBd img = Pnf<double>::compute_unfiltered_IMG(w_size, im_size, n, color_map);

            elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
            std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

            if(_16bits)
                IO::save01_in_u16(img, filename);
            else
                IO::save01_in_u8(img, filename);
        }
        else {
            start_chrono= std::chrono::system_clock::now();

            ImageGrayd img = Pnf<double>::compute_noise_unfiltered(w_size, im_size, n);

            elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
            std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

            if(_16bits)
                IO::save01_in_u16(img, filename);
            else
                IO::save01_in_u8(img, filename);
        }
    }
    return EXIT_SUCCESS;

}

int noiseCmGroundTruth(int argc, char **argv)
{
    if(argc < 3) usage(argv[0]);

    std::string filename("ground_truth.png");
    std::string inputname;
    int w(512),h(512);
    double z(1);
    int nb(10);

    int number_of_cosines(32);
    double fr_min(10), fr_max(20);

    bool cm(true);
    bool example(false);
    bool _16bits(false);

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",         required_argument,  nullptr,    'o'},
        {"width" ,          required_argument,  nullptr,    'w'},
        {"height",          required_argument,  nullptr,    'h'},
        {"zoomOut",         required_argument,  nullptr,    'z'},
        {"nbSampling",      required_argument,  nullptr,    'n'},
        {"example",         required_argument,  nullptr,    'e'},
        {"noColorMap",      no_argument,        nullptr,     1},
        {"numberOfCosines", required_argument,  nullptr,     2},
        {"frequenceMin",    required_argument,  nullptr,     3},
        {"frequenceMax",    required_argument,  nullptr,     4},
        {"16",              no_argument,        nullptr,     16 },
        {0,                 0,                  0,          0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:w:h:z:e:n:",long_options,&option_index)) != -1) {
        switch (opt) {
            case 1:
                cm = false;
                break;
            case 2:
                number_of_cosines = std::atoi(optarg);
                break;
            case 3:
                fr_min = std::atof(optarg);
                break;
            case 4:
                fr_max = std::atof(optarg);
                break;
            case 16:
                _16bits= true;
                break;
            case 'o':
                filename = optarg;
                break;
            case 'w':
                w = std::atoi(optarg);
                break;
            case 'h':
                h = std::atoi(optarg);
                break;
            case 'z':
                z = std::atof(optarg);
                break;
            case 'e':
                example = true;
                inputname = optarg;
                break;
            case 'n':
                nb = std::atoi(optarg);
                break;
            default:
                usage(argv[0]);

        }

    }

    ImageRGBd prefiltered;
    IO::loadu8_in_01(prefiltered, argv[optind]);
    double sigma_max = std::atof(argv[optind+1]);

    Color_map<double> color_map;
    color_map.set_filtered(prefiltered, sigma_max);

    Vec2 im_size(w,h);

    std::chrono::time_point<std::chrono::system_clock> start_chrono;
    std::chrono::duration<double> elapsed_seconds;

    if(!example)
    {
        Noise<double> n(number_of_cosines, fr_min, fr_max);
        Vec2 w_size(z,z);
        if(cm){
            start_chrono= std::chrono::system_clock::now();

            ImageRGBd img = Pnf<double>::compute_ground_truth_IMG(w_size, im_size, n, color_map, nb);

            elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
            std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

            IO::save01_in_u8(img, filename);
        }
        else {
            start_chrono= std::chrono::system_clock::now();

            ImageGrayd img = Pnf<double>::compute_noise_filtered(w_size, im_size, n, nb);

            elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
            std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

            IO::save01_in_u8(img, filename);
        }
    }
    else {
        ImageGrayd noise_example;
        if(_16bits)
            IO::loadu16_in_01(noise_example, inputname);
        else
            IO::loadu8_in_01(noise_example, inputname);

        TextureNoise<double> n;
        n.setNoise(noise_example);
        Vec2 w_size(noise_example.width()*z, noise_example.height()*z);

        if(cm){
            start_chrono= std::chrono::system_clock::now();

            ImageRGBd img = Pnf<double>::compute_ground_truth_IMG(w_size, im_size, n, color_map);

            elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
            std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

            if(_16bits)
                IO::save01_in_u16(img, filename);
            else
                IO::save01_in_u8(img, filename);
        }
        else {
            start_chrono= std::chrono::system_clock::now();

            ImageGrayd img = Pnf<double>::compute_noise_filtered(w_size, im_size, n);

            elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
            std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

            if(_16bits)
                IO::save01_in_u16(img, filename);
            else
                IO::save01_in_u8(img, filename);
        }
    }
    return EXIT_SUCCESS;

}

template <typename func_n, typename func_tn>
int noiseCmFiltering(int argc, char **argv, std::string filename, const func_n &fn, const func_tn &ftn)
{
    if(argc < 3) usage(argv[0]);

    std::string inputname;
    int w(512),h(512);
    double z(1);
    int nb(10);

    int number_of_cosines(32);
    double fr_min(10), fr_max(20);

    bool example(false);
    bool _16bits(false);

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",         required_argument,  nullptr,    'o'},
        {"width" ,          required_argument,  nullptr,    'w'},
        {"height",          required_argument,  nullptr,    'h'},
        {"zoomOut",         required_argument,  nullptr,    'z'},
        {"nbSampling",      required_argument,  nullptr,    'n'},
        {"example",         required_argument,  nullptr,    'e'},
        {"numberOfCosines", required_argument,  nullptr,     1},
        {"frequenceMin",    required_argument,  nullptr,     2},
        {"frequenceMax",    required_argument,  nullptr,     3},
        {"16",              no_argument,        nullptr,     16 },
        {0,                 0,                  0,          0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:w:h:z:e:n:",long_options,&option_index)) != -1) {
        switch (opt) {
            case 1:
                number_of_cosines = std::atoi(optarg);
                break;
            case 2:
                fr_min = std::atof(optarg);
                break;
            case 3:
                fr_max = std::atof(optarg);
                break;
            case 16:
                _16bits= true;
                break;
            case 'o':
                filename = optarg;
                break;
            case 'w':
                w = std::atoi(optarg);
                break;
            case 'h':
                h = std::atoi(optarg);
                break;
            case 'z':
                z = std::atof(optarg);
                break;
            case 'e':
                example = true;
                inputname = optarg;
                break;
            case 'n':
                nb = std::atoi(optarg);
                break;
            default:
                usage(argv[0]);

        }

    }

    ImageRGBd prefiltered;
    IO::loadu8_in_01(prefiltered, argv[optind]);
    double sigma_max = std::atof(argv[optind+1]);

    Color_map<double> color_map;
    color_map.set_filtered(prefiltered, sigma_max);


    Vec2 im_size(w,h);

    std::chrono::time_point<std::chrono::system_clock> start_chrono;
    std::chrono::duration<double> elapsed_seconds;

    if(!example)
    {
        Noise<double> n(number_of_cosines, fr_min, fr_max);
        Vec2 w_size(z,z);

        start_chrono= std::chrono::system_clock::now();

        ImageRGBd img = fn(w_size, im_size, n, color_map, nb);

        elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
        std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

        IO::save01_in_u8(img, filename);

    }
    else {
        ImageGrayd noise_example;
        if(_16bits)
            IO::loadu16_in_01(noise_example, inputname);
        else
            IO::loadu8_in_01(noise_example, inputname);

        TextureNoise<double> n;
        n.setNoise(noise_example);
        Vec2 w_size(noise_example.width()*z, noise_example.height()*z);

        start_chrono= std::chrono::system_clock::now();

        ImageRGBd img = ftn(w_size, im_size, n, color_map);

        elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
        std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

        if(_16bits)
            IO::save01_in_u16(img, filename);
        else
            IO::save01_in_u8(img, filename);
    }
    return EXIT_SUCCESS;

}

int noiseCmNaiveFiltering(int argc, char **argv)
{
    return noiseCmFiltering(argc, argv, "naive_filtering.png",
       [](  const Vec2 &w_size,
            const Vec2 &im_size,
            const Noise<double> &n,
            const Color_map<double> &cm,
            const int &nb){
        return Pnf<double>::compute_naive_filter_IMG(w_size, im_size, n, cm, nb);
    }, [](  const Vec2 &w_size,
            const Vec2 &im_size,
            const TextureNoise<double> &n,
            const Color_map<double> &cm){
        return Pnf<double>::compute_naive_filter_IMG(w_size, im_size, n, cm);
    });
}

int noiseCmHeitzFiltering(int argc, char **argv)
{
    return noiseCmFiltering(argc, argv, "Heitz_filtering.png",
       [](  const Vec2 &w_size,
            const Vec2 &im_size,
            const Noise<double> &n,
            const Color_map<double> &cm,
            const int &nb){
        return Pnf<double>::compute_good_filter_IMG(w_size, im_size, n, cm, nb);
    }, [](  const Vec2 &w_size,
            const Vec2 &im_size,
            const TextureNoise<double> &n,
            const Color_map<double> &cm){
        return Pnf<double>::compute_good_filter_IMG(w_size, im_size, n, cm);
    });
}

int noiseCmOurFiltering(int argc, char **argv)
{
    if(argc < 3) usage(argv[0]);

    std::string filename("our_filtering.png");
    int w(512),h(512);
    double z(1);

    bool _16bits(false);

    int option_index = 0;
    static struct option long_options[] = {
        {"outfile",         required_argument,  nullptr,    'o'},
        {"width" ,          required_argument,  nullptr,    'w'},
        {"height",          required_argument,  nullptr,    'h'},
        {"zoomOut",         required_argument,  nullptr,    'z'},
        {"16",              no_argument,        nullptr,     16 },
        {0,                 0,                  0,          0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "o:w:h:z:e:n:",long_options,&option_index)) != -1) {
        switch (opt) {
            case 16:
                _16bits= true;
                break;
            case 'o':
                filename = optarg;
                break;
            case 'w':
                w = std::atoi(optarg);
                break;
            case 'h':
                h = std::atoi(optarg);
                break;
            case 'z':
                z = std::atof(optarg);
                break;
            default:
                usage(argv[0]);

        }

    }

    ImageRGBd prefiltered;
    IO::loadu8_in_01(prefiltered, argv[optind]);
    double sigma_max = std::atof(argv[optind+1]);

    Color_map<double> color_map;
    color_map.set_filtered(prefiltered, sigma_max);


    Vec2 im_size(w,h);
    ImageGrayd noise_example;
    if(_16bits)
        IO::loadu16_in_01(noise_example, argv[optind+2]);
    else
        IO::loadu8_in_01(noise_example, argv[optind+2]);

    TextureNoise<double> n;
    n.setNoise(noise_example);
    Vec2 w_size(noise_example.width()*z, noise_example.height()*z);

    std::chrono::time_point<std::chrono::system_clock> start_chrono;
    std::chrono::duration<double> elapsed_seconds;

    start_chrono= std::chrono::system_clock::now();

    ImageRGBd img = Pnf<double>::compute_good_filter_IMG(w_size, im_size, n, color_map);

    elapsed_seconds = std::chrono::system_clock::now() - start_chrono;
    std::cout << "gen timing : " << elapsed_seconds.count() << " s." << std::endl;

    if(_16bits)
        IO::save01_in_u16(img, filename);
    else
        IO::save01_in_u8(img, filename);
    return EXIT_SUCCESS;

}

int main(int argc, char **argv){
    if(argc < 2) usage();

    if(!strcmp(argv[1], "histo"))
        return histo(argc-1,argv+1);
    else if (!strcmp(argv[1], "gaussianize"))
        return gaussianize(argc-1, argv+1);
    else if (!strcmp(argv[1], "getDeGaussTrans"))
        return getDegaussTrans(argc-1, argv+1);
    else if (!strcmp(argv[1], "prefilter"))
        return prefilter(argc-1, argv+1);
    else if (!strcmp(argv[1], "PSD"))
        return PSD(argc-1, argv+1);
    else if (!strcmp(argv[1], "noisePSD"))
        return noisePSD(argc-1, argv+1);
    else if (!strcmp(argv[1], "noiseCm"))
        return noiseCm(argc-1, argv+1);
    else if (!strcmp(argv[1], "noiseCmGroundTruth"))
        return noiseCmGroundTruth(argc-1, argv+1);
    else if (!strcmp(argv[1], "noiseCmNaiveFiltering"))
        return noiseCmNaiveFiltering(argc-1, argv+1);
    else if (!strcmp(argv[1], "noiseCmHeitzFiltering"))
        return noiseCmHeitzFiltering(argc-1, argv+1);
    else if (!strcmp(argv[1], "noiseCmOurFiltering"))
        return noiseCmOurFiltering(argc-1, argv+1);
    else usage("unknown command \"%s\"", argv[1]);

    return EXIT_SUCCESS;
}

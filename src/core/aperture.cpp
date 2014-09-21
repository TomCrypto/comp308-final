#include <easylogging.h>

#include "core/aperture.h"

#include <string>
#include <cmath>

aperture::aperture()
    : m_rng(m_rd()), m_fft(glm::ivec2(3072, 3072))
{
    const std::string& base = "apertures/";

    LOG(INFO) << "Loading aperture textures.";

    m_apertures.push_back(image(base + "circular.png"));
    m_apertures.push_back(image(base + "elliptical.png"));
    m_apertures.push_back(image(base + "pentagonal.png"));
    m_apertures.push_back(image(base + "hexagonal.png"));
    m_apertures.push_back(image(base + "heptagonal.png"));
    m_apertures.push_back(image(base + "octagonal.png"));
    m_apertures.push_back(image(base + "nonagonal.png"));
    m_apertures.push_back(image(base + "decagonal.png"));

    m_noise.push_back(image(base + "noise1.png"));
    m_noise.push_back(image(base + "noise2.png"));
    m_noise.push_back(image(base + "noise3.png"));
    m_noise.push_back(image(base + "noise4.png"));
    m_noise.push_back(image(base + "noise5.png"));
    m_noise.push_back(image(base + "noise6.png"));

    LOG(INFO) << "All textures loaded.";
}

image aperture::gen_aperture(const glm::ivec2& dims)
{
    std::uniform_int_distribution<> dist1(0, m_apertures.size() - 1);
    std::uniform_int_distribution<> dist2(0, m_noise.size() - 1);
    std::uniform_real_distribution<> rand(0, 1);

    // start with a random aperture

    image out = image(m_apertures[dist1(m_rng)]);

    // and add random noise to it at random locations

    // LOW-FREQUENCY DETAILS (large noise, few of them)

    const int l_noise_max = 25;
    int l_noise_num = (int)(rand(m_rng) * l_noise_max);

    for (int t = 0; t < l_noise_num; ++t)
    {
        // select a random point on the aperture
        int x = (int)(rand(m_rng) * out.width());
        int y = (int)(rand(m_rng) * out.height());

        // and a random size between some reasonable bounds
        int min_size = 5;
        int max_size = (int)(out.width() / 5);

        int size = (int)(rand(m_rng) * (max_size - min_size) + min_size);

        // resize the noise accordingly
        image noise = m_noise[dist2(m_rng)].resize(size, size).zero_pad(
            x, y, out.width() - size - x, out.height() - size - y);

        noise.negate();
        out.mul(noise);
    }
    
    // HIGH-FREQUENCY DETAILS (small noise, lots of them)
    
    const int h_noise_max = 1000;
    int h_noise_num = (int)(rand(m_rng) * h_noise_max);

    for (int t = 0; t < h_noise_num; ++t)
    {
        // select a random point on the aperture
        int x = (int)(rand(m_rng) * out.width());
        int y = (int)(rand(m_rng) * out.height());

        // and a random size between some reasonable bounds
        int min_size = 1;
        int max_size = 8;

        int size = (int)(rand(m_rng) * (max_size - min_size) + min_size);

        // resize the noise accordingly
        image noise = m_noise[dist2(m_rng)].resize(size, size).zero_pad(
            x, y, out.width() - size - x, out.height() - size - y);

        noise.negate();
        
        for (int y = 0; y < noise.height(); ++y)
        {
            glm::vec4* ptr = noise[y];

            for (int x = 0; x < noise.width(); ++x)
            {
                if (ptr->x < 1)
                    *ptr *= 0.2f;

                ++ptr;
            }
        }
        
        out.mul(noise);
    }

    out = out.resize(dims.x, dims.y);
    return out;
}

static glm::vec3 curve[81] = {
glm::vec3(0.0014,0.0000,0.0065), glm::vec3(0.0022,0.0001,0.0105), glm::vec3(0.0042,0.0001,0.0201),
glm::vec3(0.0076,0.0002,0.0362), glm::vec3(0.0143,0.0004,0.0679), glm::vec3(0.0232,0.0006,0.1102),
glm::vec3(0.0435,0.0012,0.2074), glm::vec3(0.0776,0.0022,0.3713), glm::vec3(0.1344,0.0040,0.6456),
glm::vec3(0.2148,0.0073,1.0391), glm::vec3(0.2839,0.0116,1.3856), glm::vec3(0.3285,0.0168,1.6230),
glm::vec3(0.3483,0.0230,1.7471), glm::vec3(0.3481,0.0298,1.7826), glm::vec3(0.3362,0.0380,1.7721),
glm::vec3(0.3187,0.0480,1.7441), glm::vec3(0.2908,0.0600,1.6692), glm::vec3(0.2511,0.0739,1.5281),
glm::vec3(0.1954,0.0910,1.2876), glm::vec3(0.1421,0.1126,1.0419), glm::vec3(0.0956,0.1390,0.8130),
glm::vec3(0.0580,0.1693,0.6162), glm::vec3(0.0320,0.2080,0.4652), glm::vec3(0.0147,0.2586,0.3533),
glm::vec3(0.0049,0.3230,0.2720), glm::vec3(0.0024,0.4073,0.2123), glm::vec3(0.0093,0.5030,0.1582),
glm::vec3(0.0291,0.6082,0.1117), glm::vec3(0.0633,0.7100,0.0782), glm::vec3(0.1096,0.7932,0.0573),
glm::vec3(0.1655,0.8620,0.0422), glm::vec3(0.2257,0.9149,0.0298), glm::vec3(0.2904,0.9540,0.0203),
glm::vec3(0.3597,0.9803,0.0134), glm::vec3(0.4334,0.9950,0.0087), glm::vec3(0.5121,1.0000,0.0057),
glm::vec3(0.5945,0.9950,0.0039), glm::vec3(0.6784,0.9786,0.0027), glm::vec3(0.7621,0.9520,0.0021),
glm::vec3(0.8425,0.9154,0.0018), glm::vec3(0.9163,0.8700,0.0017), glm::vec3(0.9786,0.8163,0.0014),
glm::vec3(1.0263,0.7570,0.0011), glm::vec3(1.0567,0.6949,0.0010), glm::vec3(1.0622,0.6310,0.0008),
glm::vec3(1.0456,0.5668,0.0006), glm::vec3(1.0026,0.5030,0.0003), glm::vec3(0.9384,0.4412,0.0002),
glm::vec3(0.8544,0.3810,0.0002), glm::vec3(0.7514,0.3210,0.0001), glm::vec3(0.6424,0.2650,0.0000),
glm::vec3(0.5419,0.2170,0.0000), glm::vec3(0.4479,0.1750,0.0000), glm::vec3(0.3608,0.1382,0.0000),
glm::vec3(0.2835,0.1070,0.0000), glm::vec3(0.2187,0.0816,0.0000), glm::vec3(0.1649,0.0610,0.0000),
glm::vec3(0.1212,0.0446,0.0000), glm::vec3(0.0874,0.0320,0.0000), glm::vec3(0.0636,0.0232,0.0000),
glm::vec3(0.0468,0.0170,0.0000), glm::vec3(0.0329,0.0119,0.0000), glm::vec3(0.0227,0.0082,0.0000),
glm::vec3(0.0158,0.0057,0.0000), glm::vec3(0.0114,0.0041,0.0000), glm::vec3(0.0081,0.0029,0.0000),
glm::vec3(0.0058,0.0021,0.0000), glm::vec3(0.0041,0.0015,0.0000), glm::vec3(0.0029,0.0010,0.0000),
glm::vec3(0.0020,0.0007,0.0000), glm::vec3(0.0014,0.0005,0.0000), glm::vec3(0.0010,0.0004,0.0000),
glm::vec3(0.0007,0.0002,0.0000), glm::vec3(0.0005,0.0002,0.0000), glm::vec3(0.0003,0.0001,0.0000),
glm::vec3(0.0002,0.0001,0.0000), glm::vec3(0.0002,0.0001,0.0000), glm::vec3(0.0001,0.0000,0.0000),
glm::vec3(0.0001,0.0000,0.0000), glm::vec3(0.0001,0.0000,0.0000), glm::vec3(0.0000,0.0000,0.0000)};

static glm::vec3 wavelength_rgb(float lambda)
{
    if (lambda <= 380)
        return curve[0];

    if (lambda >= 780)
        return curve[80];

    int pos = (int)((lambda - 380) / 5);
    float t = pos + 1 - (lambda - 380) / 5;

    auto xyz = curve[pos] + (curve[pos + 1] - curve[pos]) * t;

    glm::mat3 mat(2.3706743, -0.9000405, -0.4706338,
                 -0.5138850,  1.4253036,  0.0885814,
                  0.0052982, -0.0146949,  1.0093968);

    glm::vec3 rgb = mat * xyz;

    if (rgb.x < 0) rgb.x = 0;
    if (rgb.x > 1) rgb.x = 1;
    if (rgb.y < 0) rgb.y = 0;
    if (rgb.y > 1) rgb.y = 1;
    if (rgb.z < 0) rgb.z = 0;
    if (rgb.z > 1) rgb.z = 1;

    return rgb;
}

image aperture::get_cfft(const image& aperture, const glm::ivec2& dims)
{
    // first compute the power spectrum of the aperture

    image spectrum = m_fft.psf(aperture, glm::ivec2(aperture.width(), aperture.height()));
    spectrum.reproduce(channels::R);
    spectrum.normalize(false);

    // next superimpose it resized over different wavelengths

    image out(spectrum.width(), spectrum.height());

    const int samples = 40; // pass as quality parameter?
    const float scale = 0.75; // TODO: pass this as parameter later!

    for (int t = 0; t < samples; ++t)
    {
        float lambda = 700 - 300 * (float)t / samples;
        float scale = 1 * lambda / 700;

        int newX = (int)(out.width() * scale);
        int newY = (int)(out.height() * scale);
        int dx = out.width() - newX;
        int dy = out.height() - newY;
        int left = dx / 2;
        int top = dy / 2;
        int right = dx - left;
        int bottom = dy - top;

        auto ps_resized = spectrum.resize(newX, newY, FILTER_BILINEAR);
        auto color = wavelength_rgb(lambda);
        ps_resized.normalize(false);
        ps_resized.colorize(color);
        ps_resized = ps_resized.zero_pad(left, top, right, bottom);

        out.add(ps_resized);
    }

    out = out.resize(dims.x, dims.y, FILTER_BILINEAR);
    out.normalize(false);

    return out;
}

image aperture::get_flare(const image& cfft, float radius)
{
    return m_fft.convolve_disk(cfft, radius);
}
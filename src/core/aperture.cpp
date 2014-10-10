#include <easylogging.h>

#include "core/aperture.h"

#include <cmath>

#if 0
static bool is_empty_row(const image& img, int row, const glm::vec3& noise)
{
    const glm::vec4* ptr = img[row];
    auto dims = img.dims();

    for (int x = 0; x < dims.x; ++x)
    {
        if (ptr->x > noise.x)
            return false;
        if (ptr->y > noise.y)
            return false;
        if (ptr->z > noise.z)
            return false;

        ++ptr;
    }

    return true;
}

static bool is_empty_col(const image& img, int col, const glm::vec3& noise)
{
    auto dims = img.dims();

    for (int y = 0; y < dims.y; ++y)
    {
        const glm::vec4* ptr = img[y] + col;

        if (ptr->x > noise.x)
            return false;
        if (ptr->y > noise.y)
            return false;
        if (ptr->z > noise.z)
            return false;
    }

    return true;
}

// this function performs the following operations:
// 1. denoises the image by estimating the ground noise at the image
//    edges and removing it through subtraction
// 2. trims the image by removing any zero rows or columns
static glm::ivec2 trim_and_denoise(image& img)
{
    // TODO: better denoising? for now, just subtract 2 * 1e-7 arbitrarily

    auto dims = img.dims();

    glm::vec3 noise = glm::vec3(1e-8);

    // find empty rows
    int l = 0;
    int r = 0;
    int t = 0;
    int b = 0;

    while (is_empty_col(img, l, noise)) ++l;
    while (is_empty_row(img, t, noise)) ++t;
    while (is_empty_col(img, dims.x - 1 - r, noise)) ++r;
    while (is_empty_row(img, dims.y - 1 - b, noise)) ++b;

    // TODO: NEED TO KEEP TRACK OF CENTER HERE

    img = img.subregion(l, t, dims.x - 1 - r - l,
                              dims.y - 1 - b - t);

    return glm::ivec2((l + dims.x - 1 - r) / 2,
                      (t + dims.y - 1 - b) / 2);
}
#endif

aperture::aperture(fft_engine& fft)
    : m_fft(fft), m_shader("aperture.vert", "aperture.frag")
{

}

void aperture::load_aperture(const transmission_function& tf,
                             float scale)
{
    // 1024x1024 appears to be the sweet spot of
    // frequency resolution vs. leakage tradeoff
    const glm::ivec2& resize = glm::ivec2(1024);

    // Fibonacci series gives optimal accuracy when we
    // select the closest radius, for every lens flare
    const int radii[] = { 1, 2, 3, 5, 8, 13, 21, 34 };

    m_flares.clear();
    std::string path;

    switch (tf)
    {
        case PENTAGON:
            path = "pentagon.png";
            break;
        case FINGERPRINTS:
            path = "fingerprints.png";
            break;
        case GRAZED:
            path = "grazed_cut.png";
            break;
        case OCTAGON:
            path = "octagon.png";
            break;
        case SIGGRAPH:
            path = "siggraph_custom.png";
            break;
        default:
            LOG(ERROR) << "Bad aperture passed!";
            throw std::runtime_error("");
    }

    LOG(INFO) << "Now loading aperture '" << path << "'.";

    auto aperture = image("apertures/" + path)
                  .enlarge(((glm::ivec2)((glm::vec2)resize / scale)))
                  .resize(resize);

    LOG(TRACE) << "Computing point spread function.";

    auto point_spread_function = m_fft.psf(aperture, aperture.dims());
    point_spread_function.reproduce(channels::R);
    point_spread_function.normalize(false);

    LOG(TRACE) << "Computing chromatic Fourier transform.";

    auto chromatic_transform = get_cfft(point_spread_function);
    chromatic_transform.normalize(false);

    LOG(TRACE) << "Convolving chromatic Fourier transform.";

    for (auto r : radii) {
        auto convolved = get_flare(chromatic_transform, r);
        m_flares[r] = std::unique_ptr<gl::texture2D>(
            new gl::texture2D(convolved, GL_FLOAT)
        );
    }

    LOG(INFO) << "Aperture successfully loaded.";
}

static glm::vec3 curve[81] = {
    glm::vec3(0.0014,0.0000,0.0065), glm::vec3(0.0022,0.0001,0.0105),
    glm::vec3(0.0042,0.0001,0.0201), glm::vec3(0.0076,0.0002,0.0362),
    glm::vec3(0.0143,0.0004,0.0679), glm::vec3(0.0232,0.0006,0.1102),
    glm::vec3(0.0435,0.0012,0.2074), glm::vec3(0.0776,0.0022,0.3713),
    glm::vec3(0.1344,0.0040,0.6456), glm::vec3(0.2148,0.0073,1.0391),
    glm::vec3(0.2839,0.0116,1.3856), glm::vec3(0.3285,0.0168,1.6230),
    glm::vec3(0.3483,0.0230,1.7471), glm::vec3(0.3481,0.0298,1.7826),
    glm::vec3(0.3362,0.0380,1.7721), glm::vec3(0.3187,0.0480,1.7441),
    glm::vec3(0.2908,0.0600,1.6692), glm::vec3(0.2511,0.0739,1.5281),
    glm::vec3(0.1954,0.0910,1.2876), glm::vec3(0.1421,0.1126,1.0419),
    glm::vec3(0.0956,0.1390,0.8130), glm::vec3(0.0580,0.1693,0.6162),
    glm::vec3(0.0320,0.2080,0.4652), glm::vec3(0.0147,0.2586,0.3533),
    glm::vec3(0.0049,0.3230,0.2720), glm::vec3(0.0024,0.4073,0.2123),
    glm::vec3(0.0093,0.5030,0.1582), glm::vec3(0.0291,0.6082,0.1117),
    glm::vec3(0.0633,0.7100,0.0782), glm::vec3(0.1096,0.7932,0.0573),
    glm::vec3(0.1655,0.8620,0.0422), glm::vec3(0.2257,0.9149,0.0298),
    glm::vec3(0.2904,0.9540,0.0203), glm::vec3(0.3597,0.9803,0.0134),
    glm::vec3(0.4334,0.9950,0.0087), glm::vec3(0.5121,1.0000,0.0057),
    glm::vec3(0.5945,0.9950,0.0039), glm::vec3(0.6784,0.9786,0.0027),
    glm::vec3(0.7621,0.9520,0.0021), glm::vec3(0.8425,0.9154,0.0018),
    glm::vec3(0.9163,0.8700,0.0017), glm::vec3(0.9786,0.8163,0.0014),
    glm::vec3(1.0263,0.7570,0.0011), glm::vec3(1.0567,0.6949,0.0010),
    glm::vec3(1.0622,0.6310,0.0008), glm::vec3(1.0456,0.5668,0.0006),
    glm::vec3(1.0026,0.5030,0.0003), glm::vec3(0.9384,0.4412,0.0002),
    glm::vec3(0.8544,0.3810,0.0002), glm::vec3(0.7514,0.3210,0.0001),
    glm::vec3(0.6424,0.2650,0.0000), glm::vec3(0.5419,0.2170,0.0000),
    glm::vec3(0.4479,0.1750,0.0000), glm::vec3(0.3608,0.1382,0.0000),
    glm::vec3(0.2835,0.1070,0.0000), glm::vec3(0.2187,0.0816,0.0000),
    glm::vec3(0.1649,0.0610,0.0000), glm::vec3(0.1212,0.0446,0.0000),
    glm::vec3(0.0874,0.0320,0.0000), glm::vec3(0.0636,0.0232,0.0000),
    glm::vec3(0.0468,0.0170,0.0000), glm::vec3(0.0329,0.0119,0.0000),
    glm::vec3(0.0227,0.0082,0.0000), glm::vec3(0.0158,0.0057,0.0000),
    glm::vec3(0.0114,0.0041,0.0000), glm::vec3(0.0081,0.0029,0.0000),
    glm::vec3(0.0058,0.0021,0.0000), glm::vec3(0.0041,0.0015,0.0000),
    glm::vec3(0.0029,0.0010,0.0000), glm::vec3(0.0020,0.0007,0.0000),
    glm::vec3(0.0014,0.0005,0.0000), glm::vec3(0.0010,0.0004,0.0000),
    glm::vec3(0.0007,0.0002,0.0000), glm::vec3(0.0005,0.0002,0.0000),
    glm::vec3(0.0003,0.0001,0.0000), glm::vec3(0.0002,0.0001,0.0000),
    glm::vec3(0.0002,0.0001,0.0000), glm::vec3(0.0001,0.0000,0.0000),
    glm::vec3(0.0001,0.0000,0.0000), glm::vec3(0.0001,0.0000,0.0000)
};

static glm::vec3 wavelength_rgb(float lambda)
{
    if (lambda <= 380)
        return curve[0];

    if (lambda >= 775)
        return curve[79];

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

image aperture::get_cfft(const image& psf)
{
    image out(psf.dims());

    const int samples = 40; // pass as quality parameter?

    for (int t = 0; t < samples; ++t)
    {
        float lambda = 700 - 300 * (float)t / samples;
        float scale = lambda / 700;

        int newX = (int)(out.width() * scale);
        int newY = (int)(out.height() * scale);

        auto ps_resized = psf.resize(glm::ivec2(newX, newY),
                                     FILTER_BILINEAR);
        auto color = wavelength_rgb(lambda);
        ps_resized.normalize(false);
        ps_resized.colorize(color);
        ps_resized = ps_resized.enlarge(psf.dims());

        out.add(ps_resized);
    }

    return out;
}

image aperture::get_flare(const image& cfft, int radius)
{
    return m_fft.convolve_disk(cfft, radius);
}

void aperture::render(const std::vector<light>& lights,
                      const gl::texture2D& occlusion,
                      const camera& camera,
                      float w0, float i0)
{
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE);
    glViewport(0, 0, camera.dims().x, camera.dims().y);

    m_shader.bind();

    m_flares[8].get()->bind(0);
    occlusion.bind(1, GL_NEAREST, GL_NEAREST);
    m_shader.set("flare", 0);
    m_shader.set("occlusion", 1);

    m_shader.set("intensity", i0);
    m_shader.set("max_lights", 8);
    m_shader.set("viewproj", camera.proj() * camera.view());
    m_shader.set("view_pos", camera.pos());

    for (size_t t = 0; t < lights.size(); ++t) {
        m_shader.set("lights[" + std::to_string(t) + "].pos",
                     lights[t].pos);
        m_shader.set("lights[" + std::to_string(t) + "].radius",
                     lights[t].radius);
    }

    for (size_t t = 0; t < lights.size(); ++t) {
        //float radius = 17; // radius of flare texture (convolution)

        float s = 1; // by how much to scale?

        // Project light on sensor
        bool forward_facing = glm::dot(glm::normalize((glm::vec3)lights[t].pos - camera.pos() * lights[t].pos.w),
                                       glm::normalize(camera.dir())) > 0;
        glm::vec4 projected = camera.proj() * camera.view() * lights[t].pos;
        projected /= projected.w;

        float aspect = camera.aspect_ratio();

        if (forward_facing) {
            glBegin(GL_QUADS);
            glTexCoord3f(0.5f - 0.5f * s, 0.5f - 0.5f * s, (float)t);
            glVertex2f(-w0 + projected.x, -w0 * aspect + projected.y);
            glTexCoord3f(0.5f + 0.5f * s, 0.5f - 0.5f * s, (float)t);
            glVertex2f(+w0 + projected.x, -w0 * aspect + projected.y);
            glTexCoord3f(0.5f + 0.5f * s, 0.5f + 0.5f * s, (float)t);
            glVertex2f(+w0 + projected.x, +w0 * aspect + projected.y);
            glTexCoord3f(0.5f - 0.5f * s, 0.5f + 0.5f * s, (float)t);
            glVertex2f(-w0 + projected.x, +w0 * aspect + projected.y);
            glEnd();
        }
    }

    m_shader.unbind();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

#version 120

uniform sampler2D log_lum;
uniform float mip_level;
uniform float pixel_count;
uniform float exposure;

varying vec2 uv;

float luminance(vec3 color)
{
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

void main()
{
    float avg_log_lum = texture2D(log_lum, uv, mip_level).a;
    vec3 color = texture2D(log_lum, uv).rgb;

    float avg_lum = exp(avg_log_lum / pixel_count);
    float key = exposure / avg_lum;

    gl_FragColor = vec4(color.rgb * (key / (1.0 + luminance(color.rgb) * key)), 1);

    gl_FragColor = pow(gl_FragColor, vec4(1.0 / 2.2)); // TODO: check this later
}
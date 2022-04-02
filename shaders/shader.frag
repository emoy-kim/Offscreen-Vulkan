#version 460

layout (binding = 0) uniform MVP
{
    mat4 WorldMatrix;
    mat4 ViewMatrix;
    mat4 ProjectionMatrix;
} mvp;
layout (binding = 1) uniform sampler2D BaseTexture;
layout (binding = 2) uniform MateralInfo
{
   vec4 EmissionColor;
   vec4 AmbientColor;
   vec4 DiffuseColor;
   vec4 SpecularColor;
   float SpecularExponent;
} material;
layout (binding = 3) uniform LightInfo
{
   vec4 Position;
   vec4 AmbientColor;
   vec4 DiffuseColor;
   vec4 SpecularColor;
   vec3 AttenuationFactors;
   vec3 SpotlightDirection;
   float SpotlightExponent;
   float SpotlightCutoffAngle;
} light;

layout (location = 0) in vec3 position_in_ec;
layout (location = 1) in vec3 normal_in_ec;
layout (location = 2) in vec2 tex_coord;

layout(location = 0) out vec4 final_color;

const float zero = 0.0f;
const float one = 1.0f;
const vec4 global_ambient_color = vec4(0.2f, 0.2f, 0.2f, one);

bool IsPointLight(in vec4 light_position)
{
   return light_position.w != zero;
}

float getAttenuation(in vec3 light_vector)
{
   vec3 distance_scale;
   distance_scale.x = one;
   distance_scale.z = dot( light_vector, light_vector );
   distance_scale.y = sqrt( distance_scale.z );
   return one / dot( distance_scale, light.AttenuationFactors );
}

float getSpotlightFactor(in vec3 normalized_light_vector)
{
   if (light.SpotlightCutoffAngle >= 180.0f) return one;

   vec4 direction_in_ec = 
      transpose( inverse( mvp.ViewMatrix * mvp.WorldMatrix ) ) * 
      vec4(light.SpotlightDirection, 1.0f);
   vec3 normalized_direction = normalize( direction_in_ec.xyz );
   float cutoff_angle = clamp( light.SpotlightCutoffAngle, zero, 90.0f );
   float factor = dot( -normalized_light_vector, normalized_direction );
   return factor >= cos( radians( cutoff_angle ) ) ? pow( factor, light.SpotlightExponent ) : zero;
}

vec4 calculateLightingEquation()
{
   vec4 color = material.EmissionColor + global_ambient_color * material.AmbientColor;
   vec4 light_position_in_ec = mvp.ViewMatrix * light.Position;
   
   float final_effect_factor = one;
   vec3 light_vector = light_position_in_ec.xyz - position_in_ec;
   if (IsPointLight( light_position_in_ec )) {
      float attenuation = getAttenuation( light_vector );

      light_vector = normalize( light_vector );
      float spotlight_factor = getSpotlightFactor( light_vector );
      final_effect_factor = attenuation * spotlight_factor;
   }
   else light_vector = normalize( light_position_in_ec.xyz );

   if (final_effect_factor <= zero) return color;

   vec4 local_color = light.AmbientColor * material.AmbientColor;

   float diffuse_intensity = max( dot( normal_in_ec, light_vector ), zero );
   local_color += diffuse_intensity * light.DiffuseColor * material.DiffuseColor;

   vec3 halfway_vector = normalize( light_vector - normalize( position_in_ec ) );
   float specular_intensity = max( dot( normal_in_ec, halfway_vector ), zero );
   local_color += 
      pow( specular_intensity, material.SpecularExponent ) * 
      light.SpecularColor * material.SpecularColor;
   
   color += local_color;
   return color;
}

void main()
{
   final_color = texture( BaseTexture, tex_coord ).bgra;
   final_color *= calculateLightingEquation();
}
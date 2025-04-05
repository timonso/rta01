#version 330 core

#define PI 3.14159
#define FOUR_PI (4.0 * PI)

// uniform vec4 baseColor;
// uniform vec4 specularColor;
// uniform float alpha;
// uniform vec4 ambientColor;

// sunlight intensity
uniform vec3 lightColor;
uniform float lightIntensity;

uniform vec3 lightPosition;
uniform vec3 cameraPosition;

uniform vec3 planetCenter;
uniform float planetRadius;
uniform float atmosphereRadius;

uniform float rayleighFactor;
uniform float mieFactor;
uniform float gMie;
uniform vec3 kRayleigh; // TODO: divide by pow(.,4)
uniform vec3 kMie; // TODO: divide by pow(.,0.84)
uniform int outscatterSteps;
uniform int inscatterSteps;
uniform float h0Rayleigh; // TODO: scale by actual radius
uniform float h0Mie; // TODO: scale by actual radius

in vec3 worldSpacePosition;
out vec4 finalColor;

float debugLength = 0.0;

float gMie_sq = pow(gMie, 2.0);

// phase function for mie scattering
float miePhase(float cosTheta) {
    return 1.5 * ((1.0 - gMie_sq) / (2.0 + gMie_sq)) * ((1.0 + pow(cosTheta, 2.0)) / pow((1.0 + gMie_sq - 2 * gMie * cosTheta), 1.5));
}

// phase function for rayleigh scattering
float rayleighPhase(float cosTheta) {
    return 0.75 * (1.0 + pow(cosTheta, 2.0));
}

float height(vec3 pX) {
    return max(length(pX - planetCenter) - planetRadius, 0.0);
}

float opticalLength(float h, float h0) {
    return exp(-h / h0);
}

// calculate out-scattering for all 3 wavelengths
vec3 outScattering(vec3 pA, vec3 pB, vec3 kLambda, float h0) {
    vec3 strideDirection = (pB - pA) / float(outscatterSteps);
    vec3 pX = pA + strideDirection * 0.5;

    // innermost integral approximation
    float opticalDepth = 0.0;
    for (int i = 0; i < outscatterSteps; i++) {
        opticalDepth += opticalLength(height(pX), h0);
        pX += strideDirection;
    };

    // multiply optical depth approximation by stride length (Riemann sum)
    return FOUR_PI * kLambda * (opticalDepth * length(strideDirection));
}

vec2 raySphereIntersect(vec3 origin, vec3 direction, vec3 center, float radius) {
    vec3 offset = origin - center;
    float a = dot(direction, direction);
    float b = 2.0 * dot(offset, direction);
    float c = dot(offset, offset) - radius * radius;

    float discriminant = pow(b, 2.0) - 4.0 * a * c;
    if (discriminant < 0.0) {
        return vec2(1, 0);
    }

    float sqrtDiscriminant = sqrt(discriminant);
    float entry = (-b - sqrtDiscriminant) / (2.0 * a);
    float exit = (-b + sqrtDiscriminant) / (2.0 * a);

    return vec2(entry, exit);
}

// calculate in-scattering for all 3 wavelengths using out-scattering
vec3 inScattering(vec3 rayOrigin, vec3 rayDirection, float enterAtmosphere, float enterPlanet) {
    // combined scattering
    vec3 IvLambda = vec3(0.0);
    vec3 ppAOutRayleigh = vec3(0.0);
    vec3 ppAOutMie = vec3(0.0);

    float stride = (enterPlanet - enterAtmosphere) / float(inscatterSteps);
    vec3 strideDirection = rayDirection * stride;
    vec3 pX = rayOrigin + rayDirection * (enterAtmosphere + stride * 0.5);
    vec3 enterLightDirection = normalize(pX - lightPosition);

    vec3 rayleighInScattering = vec3(0.0);
    vec3 mieInScattering = vec3(0.0);

    for (int i = 0; i < inscatterSteps; i++) {
        float rayleighLength = opticalLength(height(pX), h0Rayleigh);
        float mieLength = opticalLength(height(pX), h0Mie);

        vec3 exitLightDirection = normalize(lightPosition - pX);
        vec3 ppC = pX + raySphereIntersect(pX, exitLightDirection, planetCenter, atmosphereRadius).y * exitLightDirection;
        vec3 ppCOutRayleigh = outScattering(pX, ppC, kRayleigh, h0Rayleigh);
        vec3 ppCOutMie = outScattering(pX, ppC, kMie, h0Mie);

        ppAOutRayleigh += FOUR_PI * kRayleigh * rayleighLength * stride;
        ppAOutMie += FOUR_PI * kMie * mieLength * stride;

        rayleighInScattering += rayleighLength * exp(-ppCOutRayleigh - ppAOutRayleigh);
        mieInScattering += mieLength * exp(-ppCOutMie - ppAOutMie);

        pX += strideDirection;
    }

    float cosTheta = dot(rayDirection, enterLightDirection);
    rayleighInScattering *= rayleighPhase(cosTheta) * rayleighFactor;
    mieInScattering *= miePhase(cosTheta) * mieFactor;
    IvLambda = lightColor * lightIntensity * (rayleighInScattering + mieInScattering) * stride;
    return IvLambda;
}

void main() {
    vec3 rayDirection = normalize(worldSpacePosition - cameraPosition);

    vec2 atmosphereIntersections = raySphereIntersect(cameraPosition, rayDirection, planetCenter, atmosphereRadius);

    // discard fragments that don't cover the atmosphere
    // if (atmosphereIntersections.x > atmosphereIntersections.y) {
    //     discard;
    // }

    vec2 planetIntersections = raySphereIntersect(cameraPosition, rayDirection, planetCenter, planetRadius);
    float enterAtmosphere = atmosphereIntersections.x;
    float enterPlanet = planetIntersections.x;

    vec3 IvLambda = inScattering(cameraPosition, rayDirection, enterAtmosphere, enterPlanet);
    vec4 IvLambda4 = vec4(IvLambda, 1.0);
    finalColor = pow(IvLambda4,vec4(1.0/2.2));

    // finalColor = vec4(vec3(debugLength), 1.0);
    // finalColor = vec4(h0Mie);
    // finalColor = IvLambda4;
    // finalColor = vec4(lightIntensity);
    // finalColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    // finalColor = vec4(worldSpacePosition, 1.0);
    // finalColor = vec4(rayDirection, 1.0);
}
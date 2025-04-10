#version 330 core

#define PI 3.14159
#define FOUR_PI (4.0 * PI)
#define GAMMA_INV 1 / 2.2

// sunlight intensity
uniform vec3 lightColor;
uniform float lightIntensity;
uniform float atmosphereAlpha;

uniform vec3 lightPosition;
uniform vec3 cameraPosition;

uniform vec3 surfaceColor;
uniform vec3 planetCenter;
uniform float planetRadius;
uniform float atmosphereRadius;

uniform float rayleighFactor;
uniform float mieFactor;
uniform float gMie;
uniform vec3 kRayleigh;
uniform vec3 kMie;
uniform int outscatterSteps;
uniform int inscatterSteps;
uniform float h0Rayleigh;
uniform float h0Mie;

in vec3 worldSpacePosition;
out vec4 finalColor;

float gMie_sq = pow(gMie, 2.0);

float miePhase(float cosTheta) {
    return 1.5 * ((1.0 - gMie_sq) / (2.0 + gMie_sq)) * ((1.0 + pow(cosTheta, 2.0)) / pow((1.0 + gMie_sq - 2.0 * gMie * cosTheta), 1.5));
}

float rayleighPhase(float cosTheta) {
    return 0.75 * (1.0 + pow(cosTheta, 2.0));
}

float height(vec3 pX) {
    return length(pX - planetCenter) - planetRadius;
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
        return vec2(0.0, -1.0);
    }

    float discriminant_sqrt = sqrt(discriminant);
    float entry = (-b - discriminant_sqrt) / (2.0 * a);
    float exit = (-b + discriminant_sqrt) / (2.0 * a);

    return vec2(entry, exit);
}

vec3 inScattering(vec3 rayOrigin, vec3 rayDirection, float enterAtmosphere, float enterPlanet) {
    vec3 IvLambda = vec3(0.0);
    vec3 pXpAOutRayleigh = vec3(0.0);
    vec3 pXpAOutMie = vec3(0.0);

    float stride = (enterPlanet - enterAtmosphere) / float(inscatterSteps);
    vec3 strideDirection = rayDirection * stride;
    vec3 pX = rayOrigin + rayDirection * (enterAtmosphere + 0.5 * stride);

    for (int i = 0; i < inscatterSteps; i++) {
        float rayleighLength = opticalLength(height(pX), h0Rayleigh);
        float mieLength = opticalLength(height(pX), h0Mie);

        vec3 exitLightDirection = normalize(lightPosition - pX);
        vec2 lightIntersect = raySphereIntersect(pX, exitLightDirection, planetCenter, atmosphereRadius);
        vec3 pXpC = pX + lightIntersect.y * exitLightDirection;

        vec3 pXpCOutRayleigh = outScattering(pX, pXpC, kRayleigh, h0Rayleigh);
        vec3 pXpCOutMie = outScattering(pX, pXpC, kMie, h0Mie);

        pXpAOutRayleigh += FOUR_PI * kRayleigh * rayleighLength * stride;
        pXpAOutMie += FOUR_PI * kMie * mieLength * stride;

        vec3 transmittanceRayleigh = exp(-pXpCOutRayleigh - pXpAOutRayleigh);
        vec3 transmittanceMie = exp(-pXpCOutMie - pXpAOutMie);

        float cosTheta = dot(rayDirection, exitLightDirection);

        vec3 IvRayleigh = rayleighLength * rayleighPhase(cosTheta) * transmittanceRayleigh;
        vec3 IvMie = mieLength * miePhase(cosTheta) * transmittanceMie;

        IvLambda += (IvRayleigh * rayleighFactor + IvMie * mieFactor) * stride;

        pX += strideDirection;
    }

    return lightColor * lightIntensity * IvLambda;
}

vec3 surfaceScattering(vec3 rayOrigin, vec3 rayDirection, float enterAtmosphere, float enterPlanet) {
    vec3 pA = rayOrigin + rayDirection * enterAtmosphere;
    vec3 pB = rayOrigin + rayDirection * enterPlanet;

    vec3 surfaceRayleigh = surfaceColor * exp(-outScattering(pA, pB, kRayleigh, h0Rayleigh));
    vec3 surfaceMie = surfaceColor * exp(-outScattering(pA, pB, kMie, h0Mie));

    return surfaceRayleigh + surfaceMie;
}

void main() {
    vec3 rayDirection = normalize(worldSpacePosition - cameraPosition);

    vec2 atmosphereIntersections = raySphereIntersect(cameraPosition, rayDirection, planetCenter, atmosphereRadius);
    if (atmosphereIntersections.x > atmosphereIntersections.y) {
        discard;
    }
   
    float enterAtmosphere = atmosphereIntersections.x;

    vec2 planetIntersections = raySphereIntersect(cameraPosition, rayDirection, planetCenter, planetRadius);
    float enterPlanet = 0.0f;

    if (planetIntersections.y < 0.0) {
        enterPlanet = atmosphereIntersections.y;
    } else {
        enterPlanet = min(planetIntersections.x, atmosphereIntersections.y);
    }

    vec3 IvLambda = inScattering(cameraPosition, rayDirection, enterAtmosphere, enterPlanet);

    // add surface colour scattering if ray hits planet
    if (planetIntersections.y >= 0.0 && planetIntersections.x > 0.0) {
        IvLambda += surfaceScattering(cameraPosition, rayDirection, enterAtmosphere, planetIntersections.x);
    }

    vec4 IvLambda4 = vec4(IvLambda, atmosphereAlpha);
    finalColor = pow(IvLambda4, vec4(GAMMA_INV));
}
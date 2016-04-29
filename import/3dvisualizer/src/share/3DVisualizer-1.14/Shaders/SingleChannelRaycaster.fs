/***********************************************************************
Fragment shader for GPU-based single-channel raycasting
Copyright (c) 2009-2010 Oliver Kreylos

This file is part of the 3D Data Visualizer (Visualizer).

The 3D Data Visualizer is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

The 3D Data Visualizer is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the 3D Data Visualizer; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
***********************************************************************/

uniform vec3 mcScale;
uniform sampler2D depthSampler;
uniform mat4 depthMatrix;
uniform vec2 depthSize;
uniform vec3 eyePosition;
uniform float stepSize;
uniform sampler3D volumeSampler;
uniform sampler1D colorMapSampler;

varying vec3 mcPosition;
varying vec3 dcPosition;

/* Brown Specific */
uniform int gridMode;
uniform float gridSize;
uniform float gridWidth;
uniform int lightMode;

uniform vec3 lightPosition;


vec3 blinn_phong(vec3 N, vec3 V, vec3 L, int light)
{
    // material properties
    // you might want to put this into a bunch or uniforms
    vec3 Ka = vec3(1.0, 1.0, 1.0);
    vec3 Kd = vec3(1.0, 1.0, 1.0);
    vec3 Ks = vec3(1.0, 1.0, 1.0);
    float shininess = 50.0;

    // diffuse coefficient
    float diff_coeff = max(abs(dot(L,N)),0.0);

    // specular coefficient
    vec3 H = normalize(L+V);
    float spec_coeff = pow(max(abs(dot(H,N)), 0.0), shininess);
    if (diff_coeff <= 0.0) 
        spec_coeff = 0.0;

    // final lighting model
    return  Ka * gl_LightSource[light].ambient.rgb + 
            Kd * gl_LightSource[light].diffuse.rgb  * diff_coeff + 
            Ks * gl_LightSource[light].specular.rgb * spec_coeff ;
}

vec3 get_gradient(vec3 samplePos, float gradientStep, vec3 normScale)
{
	float gradX = texture3D(volumeSampler,samplePos+(vec3(gradientStep,0,0)*normScale)).a - texture3D(volumeSampler,samplePos+(vec3(-gradientStep,0,0)*normScale)).a;
	float gradY = texture3D(volumeSampler,samplePos+(vec3(0,gradientStep,0)*normScale)).a - texture3D(volumeSampler,samplePos+(vec3(0,-gradientStep,0)*normScale)).a;
	float gradZ = texture3D(volumeSampler,samplePos+(vec3(0,0,gradientStep)*normScale)).a - texture3D(volumeSampler,samplePos+(vec3(0,0,-gradientStep)*normScale)).a;
	return -vec3(gradX, gradY, gradZ);
}

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
	{
	/* Calculate the ray direction in model coordinates: */
	vec3 mcDir=mcPosition-eyePosition;
	
	/* Get the distance from the eye to the ray starting point: */
	float eyeDist=length(mcDir);
	
	/* Normalize and multiply the ray direction with the current step size: */
	mcDir=normalize(mcDir);
	mcDir*=stepSize;
	eyeDist/=stepSize;
	
	/* Get the fragment's ray termination depth from the depth texture: */
	float termDepth=2.0*texture2D(depthSampler,gl_FragCoord.xy/depthSize).x-1.0;
	
	/* Calculate the maximum number of steps based on the termination depth: */
	vec4 cc1=depthMatrix*vec4(mcPosition,1.0);
	vec4 cc2=depthMatrix*vec4(mcDir,0.0);
	float lambdaMax=-(termDepth*cc1.w-cc1.z)/(termDepth*cc2.w-cc2.z);
	
	/* Convert the ray direction to data coordinates: */
	vec3 dcDir=mcDir*mcScale;
	
	/* Cast the ray and accumulate opacities and colors: */
	vec4 accum=vec4(0.0,0.0,0.0,0.0);
	
	/* Move the ray starting position forward to an integer multiple of the step size: */
	vec3 samplePos=dcPosition;
	float lambda=ceil(eyeDist)-eyeDist;
	
	vec3 sampleMult=samplePos;
	vec3 sampleMultMod=samplePos;
	float gridDist=0.0;

	vec3 gradient = vec3(0.0);
	vec3 normGradient = vec3(0.0);
	bool gradCalculated = false; 

	int temp_gridMode = 1;
	float temp_gridSize = 10.0;
	float temp_gridWidth = 0.25;
	int temp_lightMode = 0;
	
	if(lambda<lambdaMax)
		{
		samplePos+=dcDir*(lambda+rand(gl_FragCoord.xy));

		for(int i=0;i<1500;++i)
			{
			gradCalculated = false;

			/* Get the volume data value at the current sample position: */
			vec4 vol=texture1D(colorMapSampler,texture3D(volumeSampler,samplePos).a);
			
			float largestDim = max(max(mcScale.x,mcScale.y),mcScale.z);
			vec3 normScale = mcScale/largestDim;
			float gradientStep = 0.002;			
						
			
			/* Identify Grid or Checkerboard Region */
			/* TODO: plug in uniform variables */
			
			
			sampleMult = samplePos/mcScale;
			
			
			/* Line Grid */
			if (temp_gridMode == 1)
				{
				sampleMultMod = mod(sampleMult+2.5,temp_gridSize);
				gridDist=min(min(sampleMultMod.x,sampleMultMod.y),sampleMultMod.z);
				
				if(gridDist < temp_gridWidth)
					{
						gradient = get_gradient(samplePos, gradientStep, normScale);
						normGradient = normalize(gradient);
						gradCalculated = true;

						vec3 directionalWidth = ((vec3(1.0) - abs(normGradient)) * 0.85 * temp_gridWidth) + 0.15;
						if (min(directionalWidth, sampleMultMod) != directionalWidth)
						{
							vol.rg = vol.rg * 0.5;
							vol.b = vol.b *0.75;
						}
					}
				}
			
			/* Checkerboard Grid */
			if (temp_gridMode == 2)
				{
				vec3 checkersRegion = floor(sampleMult/temp_gridSize);
				float checkersColor = floor(mod(checkersRegion.x+checkersRegion.y+checkersRegion.z, 2.0));
				
				if(checkersColor == 0.0)
					{
						vol.rgb = vol.rgb * 0.75;
					}
				}
			
			if (vol.a > 0.05)
				{				
				if (!gradCalculated)
				{
					gradient = get_gradient(samplePos, gradientStep, normScale);
					normGradient = normalize(gradient);
					gradCalculated = true;
				}
				
				float magnitude = length(gradient);
				vec3 L = normalize(samplePos/mcScale - lightPosition);
				vol.rgb = vol.rgb * blinn_phong(normGradient,normalize(mcDir),L,0);
				}
			
			/* Accumulate color and opacity: */
			accum+=vol*(1.0-accum.a);
			
			/* Bail out when opacity hits 1.0: */
			if(accum.a>=1.0-1.0/256.0||lambda>=lambdaMax)
				break;

			/* Advance the sample position: */
			samplePos+=dcDir;
			lambda+=1.0;
			}
		}
	
	/* Assign the final color value: */
	gl_FragColor=accum;
	}

#version 420


in vec3 			EntryPoint;
in vec3 			FragPos;
in mat4 			View;
in mat4             Model;

uniform mat4        fcview;
uniform mat4        fcmodel;

uniform sampler2D   Transfunc;
uniform sampler2D 	ExitPoints;
uniform sampler3D   VolumeTex;


uniform vec2      	ScreenSize;
uniform vec3      	VolumeSize;


uniform vec3        BackgroundColor;
// uniform vec3      	ViewPos;

uniform float       Range[6];

uniform vec3      	LightColor;
uniform vec3        MaterialColor;
uniform vec4        LightMat;
uniform vec3      	LightPos;
uniform ivec2       NeedLightAndShadow;

uniform float       ShadowScale;

out vec4			FragColor;

uniform sampler3D 	VolumeTex0;
uniform sampler3D 	VolumeTex1;
uniform sampler3D 	VolumeTex2;
uniform sampler3D 	VolumeTex3;
uniform sampler3D 	VolumeTex4;
uniform sampler3D 	VolumeTex5;
uniform sampler3D 	VolumeTex6;
uniform sampler3D 	VolumeTex7;

uniform vec3      	ObjectColor0;
uniform vec3      	ObjectColor1;
uniform vec3      	ObjectColor2;
uniform vec3      	ObjectColor3;
uniform vec3      	ObjectColor4;
uniform vec3      	ObjectColor5;
uniform vec3      	ObjectColor6;
uniform vec3      	ObjectColor7;

// uniform float       Translate0;
// uniform float       Translate1;
// uniform float       Translate2;
// uniform float       Translate3;
// uniform float       Translate4;
// uniform float       Translate5;
// uniform float       Translate6;
// uniform float       Translate7;

// Some nasty constants
const float shadowBias = 4;
const float standardStepSize = 0.008;
const float shadowAlphaMultiplier = 5;
const float emptySpaceSkippingTreshold = 0.001;
const mat4  inverseMatrix = inverse(fcmodel) * inverse(fcview) * View * Model;
const mat4  inverseMatrix2 = inverse(fcview) * View;

float voxelSpacedStepSize(vec3 dir)
{
	vec3 voxelSize = 1.0/VolumeSize;
	vec3 result = voxelSize * dir;
	return length(result);
}

vec4 normalOnRawData(vec3 pos, float offset, int border, sampler3D volumeTex)
{
	vec3 nrm = vec3(0,0,0);
	// Get values from volume;
	float x1 = texture(volumeTex, vec3(pos.x + offset, pos.y, pos.z)).r;
	float x2 = texture(volumeTex, vec3(pos.x - offset, pos.y, pos.z)).r;
	float y1 = texture(volumeTex, vec3(pos.x, pos.y + offset, pos.z)).r;
	float y2 = texture(volumeTex, vec3(pos.x, pos.y - offset, pos.z)).r;
	float z1 = texture(volumeTex, vec3(pos.x, pos.y, pos.z + offset)).r;
	float z2 = texture(volumeTex, vec3(pos.x, pos.y, pos.z - offset)).r;

	// Create good normals at extent
	// TODO: border
	if ((border & 1) == 1)
	{
		x2 = 0;
	}
	else if ((border & 2) == 2)
	{
		x1 = 0; 
	}
	if ((border & 4) == 4)
	{
		y2 = 0;
	}
	else if ((border & 8) == 8)
	{
		y1 = 0;
	}
	if ((border & 16) == 16)
	{
		z2 = 0;
	}
	else if ((border & 32) == 32)
	{
		z1 = 0;
	}

	// Gradient
	nrm.x = (x1 - x2);
	nrm.y = (y1 - y2);
	nrm.z = (z1 - z2);

	// Use length as magnitude, divided through maximal length of sqrt(3)
	float mag = length(nrm) / 1.7320508;

	// Transformate with ModelScale;
	// TODO: multiple Volume Scale;

	nrm = normalize(nrm);
	return vec4(nrm, mag);

}

vec3 calcLighting(vec3 col, vec3 nrm, vec4 mat, vec3 dir, float shadow, vec3 pos, int border,vec3 materialColor, vec3 lightColor)
{
	// Brightness of direct light
	float brightness = 1.0 - shadow * ShadowScale;

	// Lighting (Phone)
	// TODO: ambient color
	// vec3 lightDir = normalize(LightPos - pos);

	// vec3 light = mat.x * materialColor + LightColor * max(dot(-lightDir, nrm), 0) * brightness;
	// vec3 spec = LightColor * pow(max(0.0, dot(reflect(lightDir, nrm), dir)), mat.w) * brightness;

	// Combine light and color
	// vec3 result = col * light;

	// Add specular lighting
	// result += ((col * mat.z + (1 - mat.z) * (col.r + col.g + col.b) / 3 ) * mat.y * spec);


	// Lighting (Phone)
	// 基础光照
	vec3 ambient = materialColor * col * mat.x * brightness;

	// 散射光 for 轮廓和阴影
	// vec3 lightDir = normalize(FragPos - LightPos);
	vec3 lightDir = normalize(-LightPos);
	lightDir = (transpose(View) * vec4(lightDir, 1.0f)).xyz;
	//lightDir = (transpose(View) * vec4(lightDir, 1.0f)).xyz;
	// lightDir = normalize(mat3(transpose(inverse(View))) * lightDir);
	vec3 diffuse = lightColor * col * max(dot(nrm, lightDir), 0.0f) * mat.y * brightness * 2.0;

	//  反射光
	vec3 reflectDir = reflect(-lightDir, nrm);
	vec3 specular = lightColor * col * pow(max(dot(lightDir, reflectDir), 0.0f), mat.w) * mat.z * brightness;
	
	// vec3 result;
	// if (border != 0) 
	// {
	// 	result = 1.0f;
	// }
	// else 
	// {
	// 	result = ambient + diffuse + specular;
	// }
	
	vec3 result = ambient + diffuse + specular;

	// Some clamping
	return min(result, 1);
	// return nrm;

	if (border != 0) 
	{
		return vec3(1.0f);
	}
	else 
	{
		return min(result, 1);
	}	
}

int positionInBorder(vec3 pos, vec3 offset)
{
	int border = 0;

	// vec4 extend = inverse(fcmodel) * inverse(fcview) * View * Model * vec4(pos, 1.0f);
	vec4 extend = inverseMatrix * vec4(pos, 1.0f);
	
	if (extend.x < Range[0] || extend.x > Range[1] || extend.y < Range[2] || extend.y > Range[3] || extend.z < Range[4] || extend.z > Range[5])
	{
		return -1;
	}

	// vec4 extendOffset = inverse(fcmodel) * inverse(fcview) * View * Model * vec4(offset, 1.0f);
	vec3 next = pos - offset;
	vec4 extendOffset = inverseMatrix * vec4(next, 1.0f);
	if (extendOffset.x < Range[0])
	{
		border += 1;
	}
	else if (extendOffset.x  > Range[1])
	{
		border += 2;
	}
	if (extendOffset.y < Range[2])
	{
		border += 4;
	}
	else if (extendOffset.y > Range[3])
	{
		border += 8;
	}
	if (extendOffset.z < Range[4])
	{
		border += 16;
	}
	else if (extendOffset.z > Range[5])
	{
		border += 32;
	}

	return border;
}

vec4 blend(vec4 color1, vec4 color2)
{
	vec4 color;
	color.r = color1.r * color1.a + color2.r * color2.a * (1 - color1.a);
	color.g = color1.g * color1.a + color2.g * color2.a * (1 - color1.a);
	color.b = color1.b * color1.a + color2.b * color2.a * (1 - color1.a);
	color.a = 1 - (1 - color1.a) * (1 - color2.a);
	color.r = color.r / color.a;
	color.g = color.g / color.a;
	color.b = color.b / color.a;
	return color;
}

float rayLength(vec3 pos, vec3 dir)
{
	float rayLength = 10000;
	float t;
	vec3 inversPos =  (inverseMatrix * vec4(pos, 1.0f)).xyz;
	// vec3 inversDir = normalize((inverseMatrix * vec4(dir, 1.0f)).xyz);
	vec3 inversDir = normalize((inverseMatrix2 * vec4(dir, 1.0f)).xyz);
	if (inversDir.z !=0)
	{
		// Front
		t = (Range[4] - inversPos.z) / inversDir.z;
		if (t > 0)
		{
			rayLength = min(rayLength, t);
		}
		 
		// Back
		t = (Range[5] - inversPos.z) / inversDir.z;
		if (t > 0)
		{
			rayLength = min(rayLength, t);
		}
	}
	if (inversDir.x != 0)
	{
		// Left
		t = (Range[0] - inversPos.x) / inversDir.x;
		if (t > 0)
		{
			rayLength = min(rayLength, t);
		}

		// Right
		t = (Range[1] - inversPos.x) / inversDir.x;
		if (t > 0)
		{
			rayLength = min(rayLength, t);
		}
	}
	if (inversDir.y != 0)
	{
		// Top
		t = (Range[2] - inversPos.y) / inversDir.y;
		if (t > 0)
		{
			rayLength = min(rayLength, t);
		}

		// Button
		t = (Range[3] - inversPos.y) / inversDir.y;
		if (t > 0)
		{
			rayLength = min(rayLength, t);
		}
	}

	return rayLength;
}

/** Cast ray into direction of sun */
float castShadowRay(
	vec3 pos,
	vec3 sunDir,
	float shadowThreshold,
	float stepSize,
	int border)
{
	// Initializations
	float dst = 0;
	float src;
	float currValue = 0;
	float currRayLength = 0;
	float importance;
	int innerIterations = 50;

	

	// sunDir = normalize(pos - LightPos);
	sunDir = normalize(-LightPos);
	sunDir = (transpose(View) * vec4(sunDir, 1.0f)).xyz;

	stepSize = voxelSpacedStepSize(sunDir) * shadowBias;
	// sunDir = normalize(transpose(View) * vec4(-sunDir, 1.0f)).rgb;

	float maxRayLength = rayLength(pos, -sunDir);
	// return min(maxRayLength, 1);
	vec3 currPos = pos - sunDir * stepSize * shadowBias;
	currPos = pos - sunDir * stepSize;
	currRayLength += stepSize;

	// Raycasting loop
	while (dst <= shadowThreshold)
	{
		if (currRayLength >= maxRayLength)
		{
			break;
		}
		//for(int i=0; i<innerIterations; i++)
		//{
			//border = positionInBorder(currPos, sunDir * stepSize);
			//if (border == -1)
			//{
			//	currPos -= sunDir * stepSize;
			//	currRayLength += stepSize;
			//	continue;
			//}
			currValue = texture(VolumeTex, currPos).r;
			src = texture(Transfunc, vec2(currValue, currValue)).a;

			dst += (1 - pow(1 - src, stepSize / standardStepSize)) * 2;
			currPos -= sunDir * stepSize;
			currRayLength += stepSize;
		//}


	}

	return min(dst, 1);
}

void main()
{
	// white topright EntryPoint black bottomleft fcEntryPoint
	// 如果前后景一致 抛弃当前片元
	vec3 exitPoint = texture(ExitPoints, gl_FragCoord.xy/ScreenSize).xyz;
	

	//FragColor = vec4(EntryPoint, 1.0f);
	if (EntryPoint == exitPoint)
		discard;

	vec3 dir = normalize(exitPoint - EntryPoint);

	float stepSize = voxelSpacedStepSize(dir);
	float alphaThreshold = 0.98;
	
	// Some variable reservations
	vec4 src;
	vec4 dst = vec4(0,0,0,0);
	float currRayLength = 0;
	vec4 nrm;
	float currValue;
	vec3 col;
	vec4 mat;
	float positionInRay;
	float shadow;
	int border = 0;
	vec4 extend;
	int needLight = NeedLightAndShadow.x;
	int needShadow = NeedLightAndShadow.y;

	// Further variables...
	float prevValue;
	vec3 currPos = EntryPoint;
	float nrmCalulationOffset = stepSize;
	float maxRayLength = length(exitPoint - EntryPoint);
	float innerIterations = maxRayLength / stepSize;

	vec3 dymanicLightPos = normalize(LightPos);
	
	float colorSample0, colorSample1, colorSample2, colorSample3,colorSample4,colorSample5,colorSample6,colorSample7;

	vec3 lightColor = LightColor;
	vec3 materialColor = MaterialColor;
	float transfuncIndex = 0.0f;

	for(int i=0; i<innerIterations; i++)
	{
		positionInRay = currRayLength/maxRayLength;

		border = positionInBorder(currPos, dir * stepSize);
		if (border == -1)
		{
			currRayLength += stepSize;
			currPos += dir * stepSize;
			continue;
		}

		colorSample0 = texture(VolumeTex0, currPos).r;
		colorSample1 = texture(VolumeTex1, currPos).r;
		colorSample2 = texture(VolumeTex2, currPos).r;
		colorSample3 = texture(VolumeTex3, currPos).r;
		colorSample4 = texture(VolumeTex4, currPos).r;
		colorSample5 = texture(VolumeTex5, currPos).r;
		colorSample6 = texture(VolumeTex6, currPos).r;
		colorSample7 = texture(VolumeTex7, currPos).r;

		if (ObjectColor7 != vec3(0.0f) && colorSample7 > 0.51f) 
		{
			currValue = texture(VolumeTex7, currPos).r;
			// Calculate normal if necessary
			nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex7).rgba;
			materialColor = ObjectColor7;
			lightColor = ObjectColor7;
			transfuncIndex = 8.0f;
		}
		else if (ObjectColor6 != vec3(0.0f) && colorSample6 > 0.51f)
		{
			currValue = texture(VolumeTex6, currPos).r;
			// Calculate normal if necessary
			nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex6).rgba;
			materialColor = ObjectColor6;
			lightColor = ObjectColor6;
			transfuncIndex = 7.0f;
		}
		else if (ObjectColor5 != vec3(0.0f) && colorSample5 > 0.51f)
		{
			currValue = texture(VolumeTex5, currPos).r;
			// Calculate normal if necessary
			nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex5).rgba;
			materialColor = ObjectColor5;
			lightColor = ObjectColor5;
			transfuncIndex = 6.0f;
		}
		else if (ObjectColor4 != vec3(0.0f) && colorSample4 > 0.51f)
		{
			currValue = texture(VolumeTex4, currPos).r;
			// Calculate normal if necessary
			nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex4).rgba;
			materialColor = ObjectColor4;
			lightColor = ObjectColor4;
			transfuncIndex = 5.0f;
		}
		else if (ObjectColor3 != vec3(0.0f) && colorSample3 > 0.51f)
		{
			currValue = texture(VolumeTex3, currPos).r;
			// Calculate normal if necessary
			nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex3).rgba;
			materialColor = ObjectColor3;
			lightColor = ObjectColor3;
			transfuncIndex = 4.0f;
		}
		else if (ObjectColor2 != vec3(0.0f) && colorSample2 > 0.51f)
		{
			currValue = texture(VolumeTex2, currPos).r;
			// Calculate normal if necessary
			nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex2).rgba;
			materialColor = ObjectColor2;
			lightColor = ObjectColor2;
			transfuncIndex = 3.0f;
		}
		else if (ObjectColor1 != vec3(0.0f) && colorSample1 > 0.51f)
		{
			currValue = texture(VolumeTex1, currPos).r;
			// Calculate normal if necessary
			nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex1).rgba;
			materialColor = ObjectColor1;
			lightColor = ObjectColor1;
			transfuncIndex = 2.0f;
		}
		else if (ObjectColor0 != vec3(0.0f) && colorSample0 > 0.51f)
		{
			currValue = texture(VolumeTex0, currPos).r;
			// Calculate normal if necessary
			nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex0).rgba;
			materialColor = ObjectColor0;
			lightColor = ObjectColor0;
			transfuncIndex = 1.0f;
		}
		else
		{
			currValue = texture(VolumeTex, currPos).r;
			// Calculate normal if necessary
			nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex).rgba;
			materialColor = MaterialColor;
			lightColor = LightColor;
			transfuncIndex = 0.0f;
		}
		// currValue = texture(VolumeTex, currPos).r;
		// // Calculate normal if necessary
		// nrm = normalOnRawData(currPos, nrmCalulationOffset, border, VolumeTex).rgba;
		
		
		src = texture(Transfunc, vec2(currValue,  currValue * 1.0f/9.0f + transfuncIndex * 1.0f/9.0f));	
		if (src.a < emptySpaceSkippingTreshold)
		{
			currRayLength += stepSize;
			currPos += dir * stepSize;
			continue;
		}		
		// Normalize alpha value
		src.a = min(src.a, 0.99999f);
		src.a = 1 - pow(1 - src.a, stepSize/standardStepSize);

		// Color
		col = (1.0 - dst.a) * src.rgb * src.a;

		// Refletion
		mat = LightMat;
		if (needShadow != 0)
		{
			shadow = castShadowRay(currPos, dymanicLightPos, alphaThreshold, stepSize, border);
		}
		else
		{
			shadow = 0;
		}
		if (needLight != 0)
		{
			col = calcLighting(col, nrm.rgb, mat, dir, shadow, currPos, border, materialColor, lightColor);
			
		}
		
		// ** FINAL COMPOSTION ***

		// Composition
		dst.rgb += col;
		dst.a += (1.0 - dst.a) * src.a;

		currPos += dir * stepSize;
		currRayLength += stepSize;

		if (dst.a > alphaThreshold || currRayLength >= maxRayLength)
		{
			break;
		}
	}

	// Final output

	FragColor = blend(dst, vec4(BackgroundColor, 1.0));
}














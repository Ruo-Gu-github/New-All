#version 420


in vec3 			EntryPoint;
in vec3 			FragPos;
in mat4 			View;

uniform sampler2D 	ExitPoints;

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

uniform float       Translate0;
uniform float       Translate1;
uniform float       Translate2;
uniform float       Translate3;
uniform float       Translate4;
uniform float       Translate5;
uniform float       Translate6;
uniform float       Translate7;

uniform vec2      	ScreenSize;
uniform vec3      	VolumeSize;

uniform vec3      	LightColor;
uniform vec3        BackgroundColor;
uniform vec3      	LightPos;
uniform vec3      	ViewPos;


out vec4			FragColor;

vec4 caculateOnePixel(in sampler3D 	VolumeTex, in vec3 voxelCoord, in vec3 ObjectColor, in float Translate)
{
	//周围点
	vec3  normal;
	// 体纹理每个像素的大小
	vec3  deltaOffset = 1.0f/VolumeSize;
	ivec3 ptInTex = ivec3(voxelCoord * VolumeSize + vec3(0.5f));
	

	
	float ptdLeft = 	texture(VolumeTex, vec3(ptInTex + ivec3(2, 0, 0))/VolumeSize).x;
	float ptdTop = 		texture(VolumeTex, vec3(ptInTex + ivec3(0, 2, 0))/VolumeSize).y;
	float ptdFront =	texture(VolumeTex, vec3(ptInTex + ivec3(0, 0, 2))/VolumeSize).z;

	float ptdRight = 	texture(VolumeTex, vec3(ptInTex + ivec3(-2, 0, 0))/VolumeSize).x;
	float ptdBottom = 	texture(VolumeTex, vec3(ptInTex + ivec3(0, -2, 0))/VolumeSize).y;
	float ptdBack =		texture(VolumeTex, vec3(ptInTex + ivec3(0, 0, -2))/VolumeSize).z;
	
	vec4  ptCenter = 	texture(VolumeTex, vec3(ptInTex)/VolumeSize);
	
	
	vec3 exitPoint = texture(ExitPoints, gl_FragCoord.xy/ScreenSize).xyz;
	vec3 dir = normalize(exitPoint - EntryPoint);
	
	
	if (abs(ptdRight - ptCenter.x) > abs(ptCenter.x - ptdLeft))
		normal.x = ptdRight - ptCenter.x;
	else
		normal.x = ptCenter.x - ptdLeft;		
	if (abs(ptdBottom - ptCenter.y) > abs(ptCenter.y - ptdTop))
		normal.y = ptdBottom - ptCenter.y;
	else
		normal.y = ptCenter.y - ptdTop;		
	if (abs(ptdBack - ptCenter.z) > abs(ptCenter.z - ptdFront))
		normal.z = ptdBack - ptCenter.z;
	else
		normal.z = ptCenter.z - ptdFront;
		
		
	normal = normalize(mat3(transpose(inverse(View))) * normal);
	
	// 求光照

	// 基础光照 25%
	vec3 ambient = LightColor * 0.25f;
	
	// 散射光 for 轮廓和阴影 70%
	vec3 lightDir = normalize(LightPos - FragPos);
	vec3 diffuse = max(dot(normal, lightDir), 0.0f) * LightColor * 0.7f;
	//vec3 diffuse = abs(dot(normal, lightDir)) * LightColor * 0.7f;
	
	// 反射光 20%
	vec3 viewDir = normalize(ViewPos - FragPos);
	vec3 reflectDir = reflect(-lightDir, normal);
	vec3 specular = pow(max(dot(viewDir, reflectDir), 0.0f), 64) * LightColor * 0.2f;
	//vec3 specular = pow(abs(dot(viewDir, reflectDir)), 64) * LightColor * 0.2f;
	
	vec3 result;// = ambient + diffuse + specular;
	result = ambient + diffuse + specular;
	
	if (result.x < 0.5f) 
		result.x = 0.5f;
	if (result.y < 0.5f) 
		result.y = 0.5f;
	if (result.z < 0.5f) 
		result.z = 0.5f;

	return vec4(result * ObjectColor, Translate);
}

vec4 blendColor(vec4 color1, vec4 color2)
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

void main ()
{
	// 如果前后景一致 抛弃当前片元
	vec3 exitPoint = texture(ExitPoints, gl_FragCoord.xy/ScreenSize).xyz;
	if (EntryPoint == exitPoint)
		discard;
		
	
	// 从入射点连接出射点并分割取得计算点 
	vec3 dir = exitPoint - EntryPoint;
	float len = length(dir);
	float mutilple = 1.732f/len;
	float stepSize = length(VolumeSize)/mutilple;
	vec3 deltaDir = dir/stepSize;
	float deltaDirLen = length(deltaDir);
	vec3 voxelCoord = EntryPoint;
	vec4 colorSample;
	vec3 ObjectColor;
	FragColor = vec4(BackgroundColor, 1.0f);
	vec4 resultColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);

	vec4 transColor = vec4(0.0f, 0.0f, 0.0f, 0.0f);
	bool get = false;
	bool gets[8];
	for (int i=0; i<gets.length(); i++)
	{
		gets[i] = false;
	}
	
	for (int i=0; i<5; i++)
	{
		voxelCoord += deltaDir;
	}
	for (int i=0; i<stepSize; i++)
	{
		vec4 colorSample0 = texture(VolumeTex0, voxelCoord);
		vec4 colorSample1 = texture(VolumeTex1, voxelCoord);
		vec4 colorSample2 = texture(VolumeTex2, voxelCoord);
		vec4 colorSample3 = texture(VolumeTex3, voxelCoord);
		vec4 colorSample4 = texture(VolumeTex4, voxelCoord);
		vec4 colorSample5 = texture(VolumeTex5, voxelCoord);
		vec4 colorSample6 = texture(VolumeTex6, voxelCoord);
		vec4 colorSample7 = texture(VolumeTex7, voxelCoord);	
		
		vec4 tmpcolor;
		
		if (ObjectColor7 != vec3(0.0f))
		{
			if ((colorSample7.a > 0.9f && !gets[7]) || (colorSample7.a < 0.7f && gets[7]))
			{	
				tmpcolor = caculateOnePixel(VolumeTex7, voxelCoord, ObjectColor7, Translate7);
				resultColor = blendColor(resultColor, tmpcolor);
				gets[7] = !gets[7];
			}
			else if( colorSample7.a > 0.9f && gets[7])
			{
				tmpcolor = vec4(ObjectColor7, pow(Translate7, 0.5f / length(VolumeSize)));
			}
		}
		if (ObjectColor6 != vec3(0.0f))
		{
			if ((colorSample6.a > 0.9f && !gets[6]) || (colorSample6.a < 0.7f && gets[6]))
			{	
				tmpcolor = caculateOnePixel(VolumeTex6, voxelCoord, ObjectColor6, Translate6);
				resultColor = blendColor(resultColor, tmpcolor);
				gets[6] = !gets[6];
			}
			else if( colorSample6.a > 0.9f && gets[6])
			{
				tmpcolor = vec4(ObjectColor6, pow(Translate6, 0.5f / length(VolumeSize)));
			}
		}
		if (ObjectColor5 != vec3(0.0f))
		{
			if ((colorSample5.a > 0.9f && !gets[5]) || (colorSample5.a < 0.7f && gets[5]))
			{	
				tmpcolor = caculateOnePixel(VolumeTex5, voxelCoord, ObjectColor5, Translate5);
				resultColor = blendColor(resultColor, tmpcolor);
				gets[5] = !gets[5];
			}
			else if( colorSample5.a > 0.9f && gets[5])
			{
				tmpcolor = vec4(ObjectColor5, pow(Translate5, 0.5f / length(VolumeSize)));
			}
		}
		if (ObjectColor4 != vec3(0.0f))
		{
			if ((colorSample4.a > 0.9f && !gets[4]) || (colorSample4.a < 0.4f && gets[4]))
			{	
				tmpcolor = caculateOnePixel(VolumeTex4, voxelCoord, ObjectColor4, Translate4);
				resultColor = blendColor(resultColor, tmpcolor);
				gets[4] = !gets[4];
			}
			else if( colorSample4.a > 0.9f && gets[4])
			{
				tmpcolor = vec4(ObjectColor4, pow(Translate4, 0.5f / length(VolumeSize)));
			}
		}
		if (ObjectColor3 != vec3(0.0f))
		{
			if ((colorSample3.a > 0.9f && !gets[3]) || (colorSample3.a < 0.7f && gets[3]))
			{	
				tmpcolor = caculateOnePixel(VolumeTex3, voxelCoord, ObjectColor3, Translate3);
				resultColor = blendColor(resultColor, tmpcolor);
				gets[3] = !gets[3];
			}
			else if( colorSample3.a > 0.9f && gets[3])
			{
				tmpcolor = vec4(ObjectColor3, pow(Translate3, 0.5f / length(VolumeSize)));
			}
		}
		if (ObjectColor2 != vec3(0.0f))
		{
			if ((colorSample2.a > 0.9f && !gets[2]) || (colorSample2.a < 0.7f && gets[2]))
			{	
				tmpcolor = caculateOnePixel(VolumeTex2, voxelCoord, ObjectColor2, Translate2);
				resultColor = blendColor(resultColor, tmpcolor);
				gets[2] = !gets[2];
			}
			else if( colorSample2.a > 0.9f && gets[2])
			{
				tmpcolor = vec4(ObjectColor2, pow(Translate2, 0.5f / length(VolumeSize)));
			}
		}
		if (ObjectColor1 != vec3(0.0f))
		{
			if ((colorSample1.a > 0.9f && !gets[1]) || (colorSample1.a < 0.7f && gets[1]))
			{	
				tmpcolor = caculateOnePixel(VolumeTex1, voxelCoord, ObjectColor1, Translate1);
				resultColor = blendColor(resultColor, tmpcolor);
				gets[1] = !gets[1];
			}
			else if( colorSample1.a > 0.9f && gets[1])
			{
				tmpcolor = vec4(ObjectColor1, pow(Translate1, 0.5f / length(VolumeSize)));
			}
		}		
		if (ObjectColor0 != vec3(0.0f))
		{
			if ((colorSample0.a > 0.9f && !gets[0]) || (colorSample0.a < 0.7f && gets[0]))
			{	
				tmpcolor = caculateOnePixel(VolumeTex0, voxelCoord, ObjectColor0, Translate0);
				resultColor = blendColor(resultColor, tmpcolor);
				gets[0] = !gets[0];
			}
			else if( colorSample0.a > 0.9f && gets[0])
			{
				tmpcolor = vec4(ObjectColor0, pow(Translate0, 0.5f / length(VolumeSize)));
			}
		}	

		voxelCoord += deltaDir;
		
		if (resultColor.a >= 1.0f)
			break;
	}
	FragColor = blendColor(resultColor, FragColor);
}
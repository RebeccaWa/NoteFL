// 2D���� ��һ�����봢����t0
Texture2D InputTexture : register(t0);
// ������״̬ ��һ��������s0
SamplerState InputSampler : register(s0);

// �������� (b0)
cbuffer RadialBlurProperties : register(b0) {
    float2  center      : packoffset(c0);
    float   magnitude   : packoffset(c0.z);
    float   samples     : packoffset(c0.w);
};

// Shader���
float4 main(
    float4 clipSpaceOutput  : SV_POSITION,
    float4 sceneSpaceOutput : SCENE_POSITION,
    float4 texelSpaceInput0 : TEXCOORD0
    ) : SV_Target {
    // ��ʼ��
    float4 color = 0;
    // ���㲽��ֵ
    float2 step = (center - texelSpaceInput0.xy) / samples * magnitude;
    // ����ÿ��������
    for(float i = 0; i < samples; ++i)  {
        // ��������ص�
        float2 pos = texelSpaceInput0.xy + step * i;
        // ��Ӳ�������
        color += InputTexture.Sample(InputSampler, pos);
    }
    // ƽ��
    return color / samples;
}

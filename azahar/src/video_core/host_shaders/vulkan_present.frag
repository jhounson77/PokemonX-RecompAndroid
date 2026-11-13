// Copyright 2022-2024 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the misc/licenses/gplv2.txt file included.

#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec2 frag_tex_coord;
layout (location = 0) out vec4 color;

layout (push_constant, std140) uniform DrawInfo {
    mat4 modelview_matrix;
    vec4 i_resolution;
    vec4 o_resolution;
    int screen_id_l;
    int screen_id_r;
    int layer;
    int reverse_interlaced;
    vec4 ui_clip;
};

layout (set = 0, binding = 0) uniform sampler2D screen_textures[3];

vec4 GetScreen(int screen_id) {
#ifdef ARRAY_DYNAMIC_INDEX
    return texture(screen_textures[screen_id], frag_tex_coord);
#else
    switch (screen_id) {
    case 0:
        return texture(screen_textures[0], frag_tex_coord);
    case 1:
        return texture(screen_textures[1], frag_tex_coord);
    case 2:
        return texture(screen_textures[2], frag_tex_coord);
    }
#endif
}

void main() {
    // Mask the real source button rather than displaying its grey crop corners.
    if (screen_id_l == 2 && reverse_interlaced == 2) {
        vec2 d=(frag_tex_coord-ui_clip.xy)/ui_clip.zw;
        if (dot(d,d)>1.0) discard;
    } else if (screen_id_l == 2 && (reverse_interlaced == 3 || reverse_interlaced == 4)) {
        vec2 n=(frag_tex_coord-ui_clip.xy)/ui_clip.zw;
        vec2 q=max(abs(vec2((n.y-.5)*2.6,n.x-.5))-vec2(1.1,.3),vec2(0));
        if (dot(q,q)>.04) discard;
    }
    color = GetScreen(screen_id_l);
    if (screen_id_l == 2 && reverse_interlaced == 4) {
        // Same-frame fields from the guest: no OCR, cached names or guessed PP.
        // Replace only the old outer bevel; keep all name/type/PP field pixels.
        vec2 n=(frag_tex_coord-ui_clip.xy)/ui_clip.zw;
        vec2 p=vec2(n.y,n.x);
        vec3 base=vec3(.075,.135,.205);
        vec3 accent=vec3(.20,.48,.68);
        float nameField=step(.18,p.y)*step(p.y,.485);
        float infoField=step(.515,p.y)*step(p.y,.905);
        if((screen_id_r&128)!=0&&p.x>.37)infoField=0.0;
        float fields=max(nameField,infoField);
        color=vec4(mix(base,color.rgb,fields),1.0);
        if(p.y<.035)color=vec4(accent,1.0);
    }
}

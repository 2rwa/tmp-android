#version 450
layout(location=0) in vec2 ndc;
layout(location=0) out vec4 outColor;
layout(set=0,binding=0,std140) uniform Globe {
    vec4 camera;        // yaw, pitch, zoom, outer shell radius
    vec4 controls;      // blend width, active count, transparent count, time
    vec4 spheres[80];   // xyz center, signed radius: positive = glass, negative = colored
} globe;
const vec3 origin=vec3(0.0);
vec2 sphereRoots(vec3 ro,vec3 rd,vec3 center,float radius){
    vec3 offset=ro-center;
    float b=dot(offset,rd),discriminant=b*b-dot(offset,offset)+radius*radius;
    if(discriminant<0.0)return vec2(-1.0);
    float r=sqrt(discriminant);return vec2(-b-r,-b+r);
}
float mergeMin(float a,float b,float width){
    float k=max(width-abs(a-b),0.0)/max(width,0.001);
    return min(a,b)-k*k*width*.25;
}
float glassField(vec3 p){
    float d=9.0;
    for(int i=0;i<80;i++){
        if(i>=int(globe.controls.y))break;
        vec4 b=globe.spheres[i];if(b.w<=0.0)continue;
        d=mergeMin(d,length(p-b.xyz)-b.w,max(globe.controls.x,.02));
    }
    return d;
}
vec2 opaqueField(vec3 p){
    float d=9.0,index=0.0;
    for(int i=0;i<80;i++){
        if(i>=int(globe.controls.y))break;
        vec4 b=globe.spheres[i];if(b.w>=0.0)continue;
        float dd=length(p-b.xyz)+b.w;
        if(dd<d){d=dd;index=float(i);}
    }
    return vec2(d,index);
}
float marchGlass(vec3 ro,vec3 rd,float limit){
    float t=0.0,last=0.0;
    float oldDist=glassField(ro);
    for(int j=0;j<72;j++){
        float d=glassField(ro+rd*t);
        if(d<.004){
            if(oldDist>0.0&&d<0.0){
                float low=last,high=t;
                for(int k=0;k<5;k++){
                    float mid=(low+high)*.5;
                    if(glassField(ro+rd*mid)<0.0)high=mid;else low=mid;
                }
                return (high+low)*.5;
            }
            return t;
        }
        last=t;oldDist=d;
        t+=clamp(d*.60,.008,.16);
        if(t>limit)break;
    }
    return -1.0;
}
float marchOpaque(vec3 ro,vec3 rd,float limit){
    float t=0.0;
    for(int j=0;j<70;j++){
        float d=opaqueField(ro+rd*t).x;
        if(d<.005)return t;
        t+=clamp(d*.72,.008,.17);
        if(t>limit)break;
    }
    return -1.0;
}
float glassExit(vec3 ro,vec3 rd){
    float t=0.0,prev=0.0;
    for(int j=0;j<78;j++){
        float d=glassField(ro+rd*t);
        if(d>=0.0&&t>.0001){
            float low=prev,high=t;
            for(int k=0;k<6;k++){
                float mid=.5*(low+high);
                if(glassField(ro+rd*mid)<0.0)low=mid;else high=mid;
            }
            return .5*(low+high);
        }
        prev=t;t+=clamp(-d*.8,.007,.13);
        if(t>4.9)break;
    }
    return -1.0;
}
vec3 glassNormal(vec3 p){
    float h=.014;
    return normalize(vec3(
        glassField(p+vec3(h,0,0))-glassField(p-vec3(h,0,0)),
        glassField(p+vec3(0,h,0))-glassField(p-vec3(0,h,0)),
        glassField(p+vec3(0,0,h))-glassField(p-vec3(0,0,h))));
}
vec3 opaqueNormal(vec3 p){
    float h=.014;
    return normalize(vec3(
        opaqueField(p+vec3(h,0,0)).x-opaqueField(p-vec3(h,0,0)).x,
        opaqueField(p+vec3(0,h,0)).x-opaqueField(p-vec3(0,h,0)).x,
        opaqueField(p+vec3(0,0,h)).x-opaqueField(p-vec3(0,0,h)).x));
}
vec3 backdrop(vec3 rd){
    vec3 sky=mix(vec3(.007,.02,.040),vec3(.045,.10,.155),clamp(rd.y*.5+.5,0.,1.));
    sky+=vec3(.18,.32,.42)*pow(max(dot(rd,normalize(vec3(-.6,.75,-.35))),0.0),36.);
    return sky;
}
vec3 colorOpaque(vec3 p,vec3 rayDir){
    float hitIndex=opaqueField(p).y;
    vec3 palette[7]=vec3[7](vec3(1.,.3,.28),vec3(1.,.68,.33),vec3(.64,.4,1.),
        vec3(.24,.85,.81),vec3(1.,.35,.7),vec3(.48,.65,1.),vec3(.98,.81,.4));
    vec3 c=palette[clamp(int(hitIndex),0,6)];
    vec3 n=opaqueNormal(p),light=normalize(vec3(-.65,.86,.8));
    float diff=max(dot(n,light),0.),spec=pow(max(dot(reflect(-light,n),-rayDir),0.),58.);
    return c*(.18+1.07*diff)+vec3(1.,.96,.89)*spec*1.35;
}
vec3 interior(vec3 ro,vec3 rd,float limit){
    float opaqueT=marchOpaque(ro,rd,limit);
    float glassT=marchGlass(ro,rd,limit);
    if(opaqueT>=0.0&&(glassT<0.0||opaqueT<glassT))return colorOpaque(ro+rd*opaqueT,rd);
    if(glassT>=0.0){
        vec3 entry=ro+rd*glassT;
        vec3 n=glassNormal(entry);
        float incidence=clamp(dot(n,-rd),0.,1.);
        vec3 inside=refract(rd,n,1./1.48);
        if(length(inside)<.01)inside=reflect(rd,n);
        vec3 insideOrigin=entry+inside*.014;
        float thickness=glassExit(insideOrigin,inside);
        vec3 through=backdrop(inside);
        if(thickness>0.){
            vec3 exitPos=insideOrigin+inside*thickness;
            vec3 rear=glassNormal(exitPos);
            vec3 outDir=refract(inside,-rear,1.48);
            if(length(outDir)<.01)outDir=reflect(inside,-rear);
            vec3 after=exitPos+outDir*.03;
            float behind=marchOpaque(after,outDir,5.);
            through=(behind>=0.)?colorOpaque(after+outDir*behind,outDir):backdrop(outDir);
        }
        float fresnel=pow(1.-incidence,4.);
        float spec=pow(max(dot(reflect(-normalize(vec3(-.5,.9,.7)),n),-rd),0.),92.);
        return through*exp(-vec3(.16,.08,.035)*max(thickness,.16))*(.90-.39*fresnel)+
            vec3(.25,.8,1.)*fresnel*1.5+vec3(1.,.98,.93)*spec*1.6;
    }
    return backdrop(rd)*.9+vec3(.009,.017,.025);
}
void main(){
    // gl_FragCoord uses Vulkan top-left coordinates, convert Y once into camera-up.
    vec2 uv=vec2(ndc.x,-ndc.y);
    float yaw=globe.camera.x, pitch=globe.camera.y;
    vec3 eye=vec3(sin(yaw)*cos(pitch),sin(pitch),cos(yaw)*cos(pitch))*7.65;
    vec3 fw=normalize(-eye),right=normalize(cross(fw,vec3(0,1,0))),up=normalize(cross(right,fw));
    vec3 rd=normalize(fw*2.13+right*uv.x*globe.camera.z+up*uv.y);
    vec3 rgb=backdrop(rd);
    vec2 shell=sphereRoots(eye,rd,origin,globe.camera.w);
    if(shell.x>0.){
        vec3 entry=eye+rd*shell.x,n=normalize(entry);
        vec3 inside=refract(rd,n,1./1.13);
        if(length(inside)<.01)inside=reflect(rd,n);
        vec2 hollow=sphereRoots(entry+inside*.003,inside,origin,2.41);
        vec3 col=backdrop(inside);
        if(hollow.x>=0.&&hollow.y>hollow.x){
            col=interior(entry+inside*(hollow.x+.008),inside,hollow.y-hollow.x-.014);
        }
        float fre=pow(1.-clamp(dot(n,-rd),0.,1.),4.);
        float highlight=pow(max(dot(reflect(-normalize(vec3(-.55,.84,.64)),n),-rd),0.),62.);
        rgb=col*(.92-.4*fre)+vec3(.30,.70,.93)*fre+vec3(1.)*highlight*1.8;
    }
    rgb=rgb/(1.+rgb*.53);
    outColor=vec4(pow(max(rgb,vec3(0.)),vec3(.9)),1.);
}

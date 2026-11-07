package com.example.websocktest.core;

/** Portable LVGL INDEXED_1BIT encoder/decoder; no JNI or OpenCV installation. */
public final class IndexedImage {
    public final int width,height;
    public final int[] pixels;
    public final byte[] binary;
    private IndexedImage(int width,int height,int[] pixels,byte[] binary) {
        this.width=width; this.height=height; this.pixels=pixels; this.binary=binary;
    }
    public static IndexedImage dither(int[] source,int width,int height) {
        if(width<1 || height<1 || width>512 || height>512 || source.length!=width*height)
            throw new IllegalArgumentException("Invalid image size");
        float[] gray=new float[source.length]; int[] pixels=new int[source.length];
        for(int i=0;i<source.length;i++) {
            int color=source[i], alpha=color>>>24;
            gray[i]=(float)((0.299*((color>>16)&255)+0.587*((color>>8)&255)+0.114*(color&255))*alpha/255.0+255-alpha);
        }
        int stride=(width+7)/8;
        byte[] binary=new byte[12+stride*height];
        int header=7|(width<<10)|(height<<21);
        for(int i=0;i<4;i++) binary[i]=(byte)(header>>>(8*i));
        binary[7]=(byte)255;
        for(int i=8;i<12;i++) binary[i]=(byte)255;
        for(int y=0;y<height;y++) for(int x=0;x<width;x++) {
            int i=y*width+x; boolean white=gray[i]>128; float error=gray[i]-(white?255:0);
            pixels[i]=white?0xffffffff:0xff000000;
            if(white) binary[12+y*stride+x/8]|=(byte)(0x80>>(x%8));
            if(x+1<width) gray[i+1]+=error*7/16;
            if(y+1<height) {
                if(x>0) gray[i+width-1]+=error*3/16;
                gray[i+width]+=error*5/16;
                if(x+1<width) gray[i+width+1]+=error/16;
            }
        }
        return new IndexedImage(width,height,pixels,binary);
    }
    public static IndexedImage decode(byte[] bytes) {
        if(bytes.length<12) throw new IllegalArgumentException("Incomplete image");
        int header=0; for(int i=0;i<4;i++) header|=(bytes[i]&255)<<(8*i);
        int width=(header>>>10)&2047,height=(header>>>21)&2047,stride=(width+7)/8;
        if((header&31)!=7 || width==0 || height==0 || width>512 || height>512 || bytes.length!=12+stride*height)
            throw new IllegalArgumentException("Unsupported LVGL image");
        int[] pixels=new int[width*height];
        for(int y=0;y<height;y++) for(int x=0;x<width;x++)
            pixels[y*width+x]=(bytes[12+y*stride+x/8]&(0x80>>(x%8)))!=0?0xffffffff:0xff000000;
        return new IndexedImage(width,height,pixels,bytes.clone());
    }
}

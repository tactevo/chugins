// lock mouse to window
GWindow.mouseMode(GWindow.MouseMode_Disabled);

4 => int WINDOW_SCALE;
4.0/3.0 => float aspect_ratio; // CRT aspect ratio w:h
GWindow.windowed(
    PureDOOM.screenWidth * WINDOW_SCALE, 
    (PureDOOM.screenWidth / aspect_ratio) $ int * WINDOW_SCALE
);

// Texture we copy the DOOM framebuffer to
TextureDesc tex_desc;
PureDOOM.screenWidth => tex_desc.width;
PureDOOM.screenHeight => tex_desc.height;
false => tex_desc.mips;
Texture tex(tex_desc); 

// simple rendergraph 
// no GGens or GScene, just blit the DOOM framebuffer directly
GG.rootPass() --> GG.outputPass();
GG.outputPass().input(tex);

// disable postprocessing and filtering
GG.outputPass().sampler(TextureSampler.nearest());
GG.outputPass().gamma(false);
GG.outputPass().tonemap(OutputPass.ToneMap_None);

// instantiate a PureDOOM
PureDOOM doom => dac;

while (1) {
        { // input 
        for (auto key : GWindow.keysDown())
            doom.keyDown(key);

        for (auto key : GWindow.keysUp())
            doom.keyUp(key);
        
        if (GWindow.mouseLeftDown()) doom.mouseDown(doom.Button_MouseLeft);
        if (GWindow.mouseRightDown()) doom.mouseDown(doom.Button_MouseRight);
        if (GWindow.mouseLeftUp()) doom.mouseUp(doom.Button_MouseLeft);
        if (GWindow.mouseRightUp()) doom.mouseUp(doom.Button_MouseRight);

        GWindow.mouseDeltaPos() => vec2 delta_pos;
        4 *=> delta_pos;
        doom.mouseMove(delta_pos.x $ int, delta_pos.y $ int);
    }

    // update (PureDOOM is locked to 35FPS)
    doom.update();

    // render
    doom.framebuffer() => int framebuff_ptr;

    // copy framebuffer into GPU texture
    tex.write(framebuff_ptr);

    GG.nextFrame() => now;
}


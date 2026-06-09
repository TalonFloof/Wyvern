void fb_init() {
    /* Since the VideoCore handles all of the nasty low-level display controller stuff,
     * the setup that we actually have to do is pretty elegant.
     *
     * The main sequence works as follows:
     * - Retrieve EDID information to determine the preferred resolution (use safe 1024x768@60p fallback if this errors out)
     * - Allocate a buffer on the VideoCore for our framebuffer
     * - Set our Virtual and Physical resolution to the size of the perferred active pixel area given by EDID
     * - Set our color depth to 32-bit ARGB
     */


}
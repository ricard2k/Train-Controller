#include "TrainDisplay.h"
#include "dial.h" // Jpeg image array

// Initialize static instance pointer for callback
TrainDisplay* TrainDisplay::instance = nullptr;

TrainDisplay::TrainDisplay(TFT_eSPI& display) :
    NEEDLE_LENGTH(35),
    NEEDLE_WIDTH(5),
    NEEDLE_RADIUS(90),
    NEEDLE_COLOR1(TFT_MAROON),
    NEEDLE_COLOR2(TFT_RED),
    DIAL_CENTRE_X(120),
    DIAL_CENTRE_Y(120),
    tft(&display),  // Store reference to the externally initialized display
    needle(&display), // Initialize needle with TFT_eSprite constructor
    spr(&display),   // Initialize spr with TFT_eSprite constructor
    buffer_loaded(false),
    spr_width(0),
    bg_color(0),
    tft_buffer(nullptr)
{
    // Set static instance for callback
    instance = this;
}

TrainDisplay::~TrainDisplay() {
    if (tft_buffer) {
        free(tft_buffer);
    }
}

void TrainDisplay::begin() {
    // Configure JPEG decoder
    TJpgDec.setSwapBytes(true);
    TJpgDec.setCallback(jpegCallback);

    // Draw the dial
    TJpgDec.drawJpg(0, 0, dial, sizeof(dial));
    tft->drawCircle(DIAL_CENTRE_X, DIAL_CENTRE_Y, NEEDLE_RADIUS - NEEDLE_LENGTH, TFT_DARKGREY);

    // Load the font and create the Sprite for reporting the value
    spr.loadFont(AA_FONT_LARGE);
    spr_width = spr.textWidth("777"); // 7 is widest numeral in this font
    spr.createSprite(spr_width, spr.fontHeight());
    bg_color = tft->readPixel(120, 120); // Get colour from dial centre
    spr.fillSprite(bg_color);
    spr.setTextColor(TFT_WHITE, bg_color, true);
    spr.setTextDatum(MC_DATUM);
    spr.setTextPadding(spr_width);
    spr.drawNumber(0, spr_width / 2, spr.fontHeight() / 2);
    spr.pushSprite(DIAL_CENTRE_X - spr_width / 2, DIAL_CENTRE_Y - spr.fontHeight() / 2);

    // Plot the label text
    tft->setTextColor(TFT_WHITE, bg_color);
    tft->setTextDatum(MC_DATUM);
    tft->drawString("(degrees)", DIAL_CENTRE_X, DIAL_CENTRE_Y + 48, 2);

    // Define where the needle pivot point is on the TFT
    tft->setPivot(DIAL_CENTRE_X, DIAL_CENTRE_Y);

    // Create the needle Sprite
    createNeedle();

    // Reset needle position to 0
    plotNeedle(0, 0);
}

bool TrainDisplay::jpegCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (instance) {
        return instance->tftOutput(x, y, w, h, bitmap);
    }
    return false;
}

bool TrainDisplay::tftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // Stop further decoding as image is running off bottom of screen
    if (y >= tft->height()) return 0;

    // This function will clip the image block rendering automatically at the TFT boundaries
    tft->pushImage(x, y, w, h, bitmap);

    // Return 1 to decode next block
    return 1;
}

void TrainDisplay::createNeedle() {
    needle.setColorDepth(16);
    needle.createSprite(NEEDLE_WIDTH, NEEDLE_LENGTH);  // create the needle Sprite

    needle.fillSprite(TFT_BLACK); // Fill with black

    // Define needle pivot point relative to top left corner of Sprite
    uint16_t piv_x = NEEDLE_WIDTH / 2; // pivot x in Sprite (middle)
    uint16_t piv_y = NEEDLE_RADIUS;    // pivot y in Sprite
    needle.setPivot(piv_x, piv_y);     // Set pivot point in this Sprite

    // Draw the red needle in the Sprite
    needle.fillRect(0, 0, NEEDLE_WIDTH, NEEDLE_LENGTH, TFT_MAROON);
    needle.fillRect(1, 1, NEEDLE_WIDTH-2, NEEDLE_LENGTH-2, TFT_RED);

    // Bounding box parameters to be populated
    int16_t min_x;
    int16_t min_y;
    int16_t max_x;
    int16_t max_y;

    // Work out the worst case area that must be grabbed from the TFT,
    // this is at a 45 degree rotation
    needle.getRotatedBounds(45, &min_x, &min_y, &max_x, &max_y);

    // Calculate the size and allocate the buffer for the grabbed TFT area
    tft_buffer = (uint16_t*) malloc(((max_x - min_x) + 2) * ((max_y - min_y) + 2) * 2);
}

void TrainDisplay::plotNeedle(int16_t angle, uint16_t ms_delay) {
    static int16_t old_angle = -120; // Starts at -120 degrees

    // Bounding box parameters
    static int16_t min_x;
    static int16_t min_y;
    static int16_t max_x;
    static int16_t max_y;

    if (angle < 0) angle = 0; // Limit angle to emulate needle end stops
    if (angle > 240) angle = 240;

    angle -= 120; // Starts at -120 degrees

    // Move the needle until new angle reached
    while (angle != old_angle || !buffer_loaded) {

        if (old_angle < angle) old_angle++;
        else old_angle--;

        // Only plot needle at even values to improve plotting performance
        if ((old_angle & 1) == 0) {
            if (buffer_loaded) {
                // Paste back the original needle free image area
                tft->pushRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
            }

            if (needle.getRotatedBounds(old_angle, &min_x, &min_y, &max_x, &max_y)) {
                // Grab a copy of the area before needle is drawn
                tft->readRect(min_x, min_y, 1 + max_x - min_x, 1 + max_y - min_y, tft_buffer);
                buffer_loaded = true;
            }

            // Draw the needle in the new position, black in needle image is transparent
            needle.pushRotated(old_angle, TFT_BLACK);

            // Wait before next update
            delay(ms_delay);
        }

        // Update the number at the centre of the dial
        spr.setTextColor(TFT_WHITE, bg_color, true);
        spr.drawNumber(old_angle+120, spr_width/2, spr.fontHeight()/2);
        spr.pushSprite(120 - spr_width / 2, 120 - spr.fontHeight() / 2);

        // Slow needle down slightly as it approaches the new position
        if (abs(old_angle - angle) < 10) ms_delay += ms_delay / 5;
    }
}
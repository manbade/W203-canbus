#ifndef IC_DISPLAY_H
#define IC_DISPLAY_H

#include <avr/pgmspace.h>
#include "string.h"
#include "can_comm.h"
#include "defines.h"

// CAN ID of AGW that communicates to the IC Display
#define SEND_CAN_ID 0x1A4

// CAN ID of IC Display back to AGW
#define RECEIVE_CAN_ID 0x1D0

// Width in pixels of display in the IC
#define DISPLAY_WIDTH_PX 56

/**
 * Class responsible for sending packets to the instrument
 * cluster to display custom text.
 * This is done by copying the AGW >> IC protocol that Mercedes
 * uses in the W203/W211/W209 cars. (AGW -> Audio gateway AKA Radio)
 * 
 * As all of this has been done via reverse engineering, I cannot
 * guarantee that I have done EVERYTHING perfectly, therefore on odd
 * occasions the Display may not act as told.
 *    Also, it is possible for the AGW to override the IC Display at times.
 * I have built mitigations to help with this. The only real solution would be to
 * disconnect the CAN Cables coming out of the AGW in the car, so that only the Arduino
 * can talk to the IC.
 * 
 * This class also deals with wrapping the AGW >> IC communication packets in ISO-15765-2,
 * so that we can send up to 55 bytes of raw data to the IC display in up to 8 consecutive packets.
 * 
 * See https://docs.google.com/spreadsheets/d/1krPDmjjwmlta4jAVcDMoWbseAokUYnBAHn67pOo00C0/edit?usp=sharing for protocol documentation
 */
class IC_DISPLAY {
    public:
        /**
         * possible pages to send text to
         */
        enum PAGE {
            AUDIO = 0x03,
            TELEPHONE = 0x05,
            OTHER = 0x00
        };

        void update();

        /**
         * Used to store the current page on the IC being active
         */
        static PAGE current_page;

        /**
         * Symbols that can be sent to the IC display
         */
        enum IC_SYMBOL {
           NONE = 0x00,
           SKIP_TRACK = 0x01, // |>>
           PREV_TRACK = 0x02, // <<|
           FAST_FWD   = 0x03, // >>
           FAST_REV   = 0x04, // <<
           PLAY       = 0x05, // ▶️
           REWIND     = 0x06, // ◀
           UP_ARROW   = 0x09, // ↑
           DOWN_ARROW = 0x0A  // ↓
        };

        IC_DISPLAY(CANBUS_COMMUNICATOR *can);

        /**
         * Returns true if body text can fit in display's width
         * @param text Pointer to char array of text to fit in the body
         */
        bool can_fit_body_text(const char* text);

        /**
         * Sets the header text by itself using Package 29
         * 
         * @param p Page to send header text to Audio / Telephone
         * @param text Header text to display on page
         * @param should_center Should the header text be centered?
         */
        void setHeader(PAGE p, const char* text, bool should_center);

        /**
         * Sends the body text to the instrument cluster using package 26
         * 
         * @param p Destination page for the Body text
         * @param text Text to display on the display
         * @param should_center Set to true if text should be centered on the display (Default)
         * if false, then text is left justified.
         */
        void setBody(PAGE p, const char* text, bool should_center);

        /**
         * To be only used by the Telephone page! (Audio page only can have 1 line of text)
         * Sends data about up to 4 lines of text to be displayed on the IC
         */
        void setBodyTel(const char* line1,const char* line2,const char* line3, const char* line4);
        
        /**
         * Sends Package 24 to display to tell it how to format the page.
         * 
         * @param p Page to initialise. Audio / Telephone
         * @param header Header text to set on init.
         * @param should_center Should the header text be centered?
         * @param upper_Symbol Symbol above body text
         * @param lower_Symbol Symbol below body text
         */
        void initPage(PAGE p, const char* header, bool should_center, IC_SYMBOL upper_Symbol, IC_SYMBOL lower_Symbol);


        /**
         * Processes an incomming can frame from the IC display
         * @param r Pointer to can frame with ID 0x1D0 that has been read from the bus
         */
        void processIcResponse(can_frame *r);
    private:
        /** Used to store payload buffer */
        uint8_t buffer[55] = { 0x00 };

        /** Used to store size of payload */
        uint8_t buffer_size = 0;

        /**
         * Sends [buffer] in a wrapped ISO 15765-2 protocol.
         * Up to a maximum of 8 can frames can be sent to the IC at a time.
         */
        void sendBytes(int pre_delay, int post_delay);

        can_frame x;

        /**
         * Returns checksum of the package being sent to the IC
         * @param len Length of payload to be sent
         * @param payload Payload data to be sent
         */
        uint8_t getChecksum(uint8_t len, uint8_t* payload);

        CANBUS_COMMUNICATOR *canB;
};

const char * const PROGMEM AGW_TO_IC_STR = "AGW >> IC: ";

/**
 * Stores widths of charcters in table (plus 1 pixel gap between each char)
 * See https://docs.google.com/spreadsheets/d/1krPDmjjwmlta4jAVcDMoWbseAokUYnBAHn67pOo00C0/edit?usp=sharing
 * for full list of Character sizes
 */
const uint8_t CHAR_WIDTHS[256] PROGMEM = {   // Global variable so PROGMEM attribute can be used  
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 7, 6, 0, 0, 0,
    0, 6, 6, 6, 7, 7, 3, 2, 
    7, 7, 0, 0,10,10, 6, 6,
    6, 3, 4, 6, 6, 6, 6, 2,
    5, 5, 6, 6, 3, 5, 2, 6,
    7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 3, 4, 5, 6, 5, 6,

    6, 7, 7, 7, 7, 6, 6, 7,
    7, 3, 5, 7, 6, 7, 0, 0,
    7, 7, 7, 7, 7, 7, 7,11,
    7, 7, 7, 4, 6, 4, 3, 6,
    3, 6, 6, 6, 6, 7, 6, 8,
    6, 3, 5, 6, 3, 9, 7, 7,
    6, 6, 6, 6, 5, 7, 7, 9,
    7, 6, 6, 6, 2, 6,99, 0, // One marked with 99 crashes the IC!

    7, 6, 8, 9, 6, 6, 6, 6,
    7, 6, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,

    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

#endif
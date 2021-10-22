/*
 *  Contains routines for user setup
 */
// this is not tested
#ifndef _DISPLAY_WRAPPER_H_
#include "src/displayWrapper.h"
#endif
#ifndef _SDI12_WRAPPER_H_
#include "src/sdi12Wrapper.h"
#endif
#ifndef _SDI12_WRAPPER_H_
#include "src/memory.h"
#endif

/*
 *  Routines for user interaction
 * - rather crude and linear but since this code will not run on the device,
 * I postbone pretty here
 */
class SetupHelpers {

  public:
    /*
     *  Inform user that changes will take effect after RESET also
     *  required to resume normal operation
     *
     *  - Passing through dspl here. Maybe we should make this part of a
     *  constructor
     */
    static void resetMessage(DisplayWrapper &dspl) {
      dspl.resetDisplay();
      dspl.printBuffer(
        "Push RESET\nto re-start device\nand start logging");
      delay(2000);
    };

    /*
     *  User menu/screen to change SDI-configuration
     */
    static void setupSDI12Addresses(
      SDI12Measurement &sdi12, DisplayWrapper &dspl
    ) {
      int ctr = 0;
      char bfr[10];
      int len = 0;
      char availableChannels[32];
      char newChannel;
      size_t numberOfChannels = 0;
      size_t idx = 0;
      while (true) {
        dspl.resetDisplay();
        dspl.printBuffer("SETUP SDI 12\n");
        numberOfChannels = sdi12.getChannels(availableChannels);
        dspl.setCursor(0, 10);
        dspl.printBuffer("<- sel addr");
        dspl.setCursor(0, 28);
        dspl.printBuffer("<- set addr\n\n");
        dspl.setCursor(0, 46);
        dspl.printBuffer("<- save");
        // dspl.printBuffer("Acquiring configuration");
        idx = 0;
        newChannel = availableChannels[idx];
        while (true) {
          // maybe we can consilidate in a screen function using a struct
          dspl.setCursor(90, 10);
          dspl.setTextColor(SH110X_BLACK);
          // the display uses an odd character page
          // see here https://learn.adafruit.com/assets/103682
          dspl.write(218);
          dspl.setTextColor(SH110X_WHITE);
          dspl.setCursor(90, 10);
          dspl.printBuffer(availableChannels+idx, 1);
          dspl.setCursor(90, 28);
          dspl.setTextColor(SH110X_BLACK);
          dspl.write(218);
          dspl.setTextColor(SH110X_WHITE);
          dspl.setCursor(90, 28);
          dspl.print(newChannel);
          if (dspl.buttonDebounced(BUTTON_A)) {
            idx++;
            if (idx == numberOfChannels) { idx = 0; }
            newChannel = availableChannels[idx];
          }
          if (dspl.buttonDebounced(BUTTON_B)) {
            newChannel++;
            if (newChannel > 57) {
              newChannel = 48;
            }
          }
          if (dspl.buttonDebounced(BUTTON_C)) {
            if (availableChannels[idx] == newChannel ||
              sdi12.setChannel(availableChannels[idx], newChannel)
            ) {
              resetMessage(dspl);
              break;
            } else {
              dspl.setCursor(90, 46);
              dspl.printBuffer("error");
            };
          }
        }
      }
    };

    /*
     *  User menu/screen to change measurement frequency
     */
    static void setupFrequency(DisplayWrapper &dspl) {
      PersistentMemory mem;
      char bfr[16];
      while (true) {
        uint32_t current = mem.getMeasurementFrequency(3600);
        uint32_t updated = current;
        uint32_t updated_old = 0;
        dspl.resetDisplay();
        dspl.printBuffer("SETUP Measrmnt Frqncy\n");
        dspl.setCursor(0, 10);
        dspl.printBuffer("<- +60   old");
        dspl.setCursor(0, 28);
        dspl.printBuffer("<- -60   new");
        dspl.setCursor(0, 46);
        dspl.printBuffer("<- save");
        // TODO: this is a bit fuzzy, we have to decide were we put the default
        // frequency eventually
        dspl.setCursor(80, 10);
        sprintf(bfr, "%d", current);
        dspl.printBuffer(bfr);
        while (true) {
          if (updated != updated_old) {
            dspl.setCursor(80, 28);
            dspl.setTextColor(SH110X_BLACK);
            char del[6] = { 218, 218, 218, 218, 218, 218 };
            dspl.printBuffer(del);
            dspl.setTextColor(SH110X_WHITE);
            dspl.setCursor(80, 28);
            // change display only if changed to avoid flicker
            sprintf(bfr, "%d", updated);
            dspl.printBuffer(bfr);
          }
          updated_old = updated;
          if (dspl.buttonDebounced(BUTTON_A)) updated += 60;
          if (dspl.buttonDebounced(BUTTON_B)) updated -= 60;
          if (updated > 86400) updated = 86400;
          if (updated < 60) updated = 60;
          if (dspl.buttonDebounced(BUTTON_C)) {
            mem.writeFrequency(updated);
            if (updated != mem.readFrequency()) {
              dspl.setCursor(80, 46);
              dspl.printBuffer("error");
            } else {
              resetMessage(dspl);
              break;
            }
          }
        }
      }
    };

 };

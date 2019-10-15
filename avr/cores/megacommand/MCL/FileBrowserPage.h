/* Justin Mammarella jmamma@gmail.com 2018 */

#ifndef FILEBROWSERPAGE_H__
#define FILEBROWSERPAGE_H__

#include "GUI.h"
#include "MCLEncoder.h"
#include "SdFat.h"
#include "SeqPage.h"

#define MAX_ENTRIES 1024

#ifdef OLED_DISPLAY
#define MAX_VISIBLE_ROWS 4
#else
#define MAX_VISIBLE_ROWS 1
#endif

#define MENU_WIDTH 78

#define MAX_FB_ITEMS 4

class FileBrowserPage : public LightPage {
public:
  //  char file_entries[NUM_FILE_ENTRIES][16];
  int numEntries;

  char match[5];
  char lwd[128];
  uint8_t cur_col = 0;
  uint8_t cur_row = 0;
  uint8_t cur_file = 0;
  bool show_dirs = false;
  bool show_save = true;
  bool show_parent = true;
  bool show_new_folder = true;
  char title[12];
  File file;

  FileBrowserPage(Encoder *e1 = NULL, Encoder *e2 = NULL, Encoder *e3 = NULL,
                  Encoder *e4 = NULL)
      : LightPage(e1, e2, e3, e4) {}
  virtual bool handleEvent(gui_event_t *event);
  virtual void display();
  void add_entry(char *entry);
  void draw_scrollbar(uint8_t x_offset);
  bool create_folder();
  virtual void loop();
  virtual void setup();
  virtual void init();

  virtual void on_new() {}
  virtual void on_select(const char *) {}
  virtual void on_delete(const char *) {}
  virtual void on_rename(const char *from, const char *to) {}
  // on cancel, the page will be popped,
  // and there's a last chance to clean up.
  virtual void on_cancel() { GUI.popPage(); }

private:
  void _calcindices(int &, int &);
  void _cd_up();
  void _cd(const char *);
};

#endif /* FILEBROWSERPAGE_H__ */

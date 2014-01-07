#include "pebble.h"

static Window *window;

static GBitmap *background_image;
static BitmapLayer *background_layer;

static GBitmap *date_format_image;
static BitmapLayer *date_format_layer;

static GBitmap *meter_bar_image;
static BitmapLayer *meter_bar_layer;
GRect meter_bar_frame;

// TODO: Handle 12/24 mode preference when it's exposed.
static GBitmap *time_format_image;
static BitmapLayer *time_format_layer;
static bool time_format_image_loaded = false;

static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;

static GBitmap *charge_image_white;
static BitmapLayer *charge_layer_white;
static GBitmap *charge_image_black;
static BitmapLayer *charge_layer_black;

#define DATE_FORMAT_PKEY 1
#define DATE_FORMAT_US 1
#define DATE_FORMAT_INT 2

static int current_date_format;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;


const int DATENUM_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9
};


#define TOTAL_DATE_DIGITS 2
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

#define TOTAL_MONTH_DIGITS 2
static GBitmap *month_digits_images[TOTAL_MONTH_DIGITS];
static BitmapLayer *month_digits_layers[TOTAL_MONTH_DIGITS];

#define TOTAL_SECONDS_DIGITS 2
static GBitmap *seconds_digits_images[TOTAL_SECONDS_DIGITS];
static BitmapLayer *seconds_digits_layers[TOTAL_SECONDS_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};


#define TOTAL_TIME_DIGITS 4
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

#define TOTAL_YEAR_DIGITS 4
static GBitmap *year_digits_images[TOTAL_YEAR_DIGITS];
static BitmapLayer *year_digits_layers[TOTAL_YEAR_DIGITS];

void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;

  *bmp_image = gbitmap_create_with_resource(resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = (*bmp_image)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);

  if (old_image != NULL) {
  	gbitmap_destroy(old_image);
  }
}


static unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }

  unsigned short display_hour = hour % 12;

  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_seconds (int seconds_after_minute) {
  set_container_image(&seconds_digits_images[0], seconds_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[seconds_after_minute/10], GPoint(111, 71));
  set_container_image(&seconds_digits_images[1], seconds_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[seconds_after_minute%10], GPoint(123, 71));
}

static void update_time (struct tm *current_time) {
  unsigned short display_hour = get_display_hour(current_time->tm_hour);

  int yPos = 70;
  // TODO: Remove leading zero?
  set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(18, yPos));
  set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(38, yPos));

  set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min/10], GPoint(65, yPos));
  set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[current_time->tm_min%10], GPoint(85, yPos));

  if (!clock_is_24h_style()) {
    if (current_time->tm_hour >= 12) {
      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), false);
      set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_PM_MODE, GPoint(7, 75));
    } else {
      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), true);
    }

    if (display_hour/10 == 0) {
    	layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
    } else {
    	layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
    }
  }
}

static void update_date (int day_of_month, int days_since_sunday) {
  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[days_since_sunday], GPoint(33, 126));

  int date_digit_ten = day_of_month/10;
  int date_digit_one = day_of_month%10;
  GPoint date_digit_ten_pos;
  GPoint date_digit_one_pos;

  if (current_date_format == DATE_FORMAT_US) {
    date_digit_ten_pos = GPoint(46, 37);
    date_digit_one_pos = GPoint(59, 37);
    layer_set_hidden(bitmap_layer_get_layer(date_digits_layers[0]), false);

  } else {

    date_digit_ten_pos = GPoint(14, 37);
    date_digit_one_pos = GPoint(27, 37);

    if (date_digit_ten == 0) {
      layer_set_hidden(bitmap_layer_get_layer(date_digits_layers[0]), true);
    } else {
	  layer_set_hidden(bitmap_layer_get_layer(date_digits_layers[0]), false);
    }
  }

  set_container_image(&date_digits_images[0], date_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[date_digit_ten], date_digit_ten_pos);
  set_container_image(&date_digits_images[1], date_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[date_digit_one], date_digit_one_pos);

}

static void update_month (int months_since_january) {
  int month_value = months_since_january + 1;
  int month_digit_ten = month_value/10;
  int month_digit_one = month_value%10;
  GPoint month_digit_ten_pos;
  GPoint month_digit_one_pos;

  if (current_date_format == DATE_FORMAT_US) {
    month_digit_ten_pos = GPoint(14, 37);
    month_digit_one_pos = GPoint(27, 37);

    if (month_digit_ten == 0) {
      layer_set_hidden(bitmap_layer_get_layer(month_digits_layers[0]), true);
    } else {
	  layer_set_hidden(bitmap_layer_get_layer(month_digits_layers[0]), false);
    }
  } else {
    month_digit_ten_pos = GPoint(46, 37);
    month_digit_one_pos = GPoint(59, 37);
    layer_set_hidden(bitmap_layer_get_layer(month_digits_layers[0]), false);
  }

  set_container_image(&month_digits_images[0], month_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[month_digit_ten], month_digit_ten_pos);
  set_container_image(&month_digits_images[1], month_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[month_digit_one], month_digit_one_pos);

}

static void update_year (int years_since_1900) {
  int display_year = years_since_1900 + 1900;
  int year_digit_thousand = display_year/1000;
  display_year -= 1000 * year_digit_thousand;

  int year_digit_hundred = display_year / 100;
  display_year -= 100 * year_digit_hundred;

  int year_digit_ten = display_year / 10;
  display_year -= 10 * year_digit_ten;

  int year_digit_one = display_year;
  set_container_image(&year_digits_images[0], year_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[year_digit_thousand], GPoint(87, 37));
  set_container_image(&year_digits_images[1], year_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[year_digit_hundred], GPoint(99, 37));
  set_container_image(&year_digits_images[2], year_digits_layers[2], DATENUM_IMAGE_RESOURCE_IDS[year_digit_ten], GPoint(111, 37));
  set_container_image(&year_digits_images[3], year_digits_layers[3], DATENUM_IMAGE_RESOURCE_IDS[year_digit_one], GPoint(123, 37));

}

static void update_display(struct tm *current_time, bool force_update) {

  update_seconds(current_time->tm_sec);

  if (current_time->tm_sec == 0 || force_update) {
  	// the hours and minutes
    update_time(current_time);

    if ((current_time->tm_hour == 0 && current_time->tm_min == 0) || force_update) {
  		// the day and date
      update_date(current_time->tm_mday, current_time->tm_wday);

	  if (current_time->tm_mday == 1 || force_update) {
    	update_month(current_time->tm_mon);

    	if (current_time->tm_mon == 0 || force_update) {
    	  update_year(current_time->tm_year);
    	}
      }
  	}
  }
}

static void vibrate_for_disconnect () {
  static const uint32_t const segments[] = { 50, 100, 50, 100, 50 };
  VibePattern pat = {
	.durations = segments,
	.num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pat);
}

static void handle_bluetooth_connection (bool isConnected) {
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), isConnected);
  if (!isConnected) {
    vibrate_for_disconnect();
  }
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  update_display(tick_time, false);
}

static void handle_battery_state (BatteryChargeState charge) {
  layer_set_hidden(bitmap_layer_get_layer(charge_layer_white), !charge.is_plugged);
  layer_set_hidden(bitmap_layer_get_layer(charge_layer_black), !charge.is_plugged);

  // a little hack for battery indicator, rounding battery up by 10%
  int altered_charge = (charge.charge_percent <= 90) ? charge.charge_percent+10 : charge.charge_percent;
  meter_bar_frame.size.w = (meter_bar_image->bounds.size.w * altered_charge) / 100;
  layer_set_frame(bitmap_layer_get_layer(meter_bar_layer), meter_bar_frame);
}

// this function will be called when the iphone app sends through the new date format
static void set_date_format (int date_format) {
  current_date_format = date_format;

  if (current_date_format == DATE_FORMAT_US) {
  	set_container_image(&date_format_image, date_format_layer, RESOURCE_ID_IMAGE_DATE_FORMAT_US, GPoint(33, 30));
  } else {
    set_container_image(&date_format_image, date_format_layer, RESOURCE_ID_IMAGE_DATE_FORMAT_INT, GPoint(33, 30));
  }

  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  update_display(tick_time, true);
}

// this function will be called when the app initialises
static void load_date_format () {

  if (persist_exists(DATE_FORMAT_PKEY)) {
    set_date_format(persist_read_int(DATE_FORMAT_PKEY));
  } else {
    // default to US
  	set_date_format(DATE_FORMAT_US);
  }
}

void out_sent_handler(DictionaryIterator *sent, void *context) {
   // outgoing message was delivered
}

void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
}

void in_received_handler(DictionaryIterator *iter, void *context) {
  // incoming message received

  Tuple *number_tuple = dict_find(iter, 0);

  int headerLength = strlen(number_tuple->value->cstring) + 1;
  char header[headerLength];
  strncpy(header, number_tuple->value->cstring, headerLength);

  // Check for fields you expect to receive
  Tuple *text_tuple = dict_find(iter, 1);

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Header: %s", header);

  if (strcmp(header, "set_date_format") == 0) {
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Setting Date Format: %d", text_tuple->value->uint8);
    set_date_format(text_tuple->value->uint8);
  } else if (strcmp(header, "error") == 0) {
  	///text_layer_set_text(status_layer, text_tuple->value->cstring);
  }
}

static void init(void) {

  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));
  memset(&date_digits_layers, 0, sizeof(date_digits_layers));
  memset(&date_digits_images, 0, sizeof(date_digits_images));
  memset(&month_digits_layers, 0, sizeof(month_digits_layers));
  memset(&month_digits_images, 0, sizeof(month_digits_images));
  memset(&year_digits_layers, 0, sizeof(year_digits_layers));
  memset(&year_digits_images, 0, sizeof(year_digits_images));
  memset(&seconds_digits_layers, 0, sizeof(seconds_digits_layers));
  memset(&seconds_digits_images, 0, sizeof(seconds_digits_images));

  window = window_create();
  if (window == NULL) {
      APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }
  window_stack_push(window, true /* Animated */);
  Layer *window_layer = window_get_root_layer(window);

  background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  background_layer = bitmap_layer_create(layer_get_frame(window_layer));
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));

  if (!clock_is_24h_style()) {
    time_format_image_loaded = true;
    time_format_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_24_HOUR_MODE);
    GRect frame = (GRect) {
      .origin = { .x = 17, .y = 68 },
      .size = time_format_image->bounds.size
    };
    time_format_layer = bitmap_layer_create(frame);
    bitmap_layer_set_bitmap(time_format_layer, time_format_image);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_format_layer));
  }

  bluetooth_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  GRect bt_frame = (GRect) {
    .origin = { .x = 107, .y = 115 },
    .size = bluetooth_image->bounds.size
  };
  bluetooth_layer = bitmap_layer_create(bt_frame);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));

  meter_bar_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_METER_BAR);
  meter_bar_frame = (GRect) {
    .origin = { .x = 13, .y = 128 },
    .size = (GSize) { .w = meter_bar_image->bounds.size.w, .h = meter_bar_image->bounds.size.h}
  };
  meter_bar_layer = bitmap_layer_create(meter_bar_frame);
  bitmap_layer_set_bitmap(meter_bar_layer, meter_bar_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(meter_bar_layer));

  charge_image_white = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGE_ICON_WHITE);
  charge_image_black = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHARGE_ICON_BLACK);

  GRect charge_frame = (GRect) {
    .origin = { .x = 16, .y = 128 },
    .size = charge_image_white->bounds.size
  };

  // Use GCompOpOr to display the white portions of the image
  charge_layer_white = bitmap_layer_create(charge_frame);
  bitmap_layer_set_bitmap(charge_layer_white, charge_image_white);
  bitmap_layer_set_compositing_mode(charge_layer_white, GCompOpOr);
  layer_add_child(window_layer, bitmap_layer_get_layer(charge_layer_white));

  // Use GCompOpClear to display the black portions of the image
  charge_layer_black = bitmap_layer_create(charge_frame);
  bitmap_layer_set_bitmap(charge_layer_black, charge_image_black);
  bitmap_layer_set_compositing_mode(charge_layer_black, GCompOpClear);
  layer_add_child(window_layer, bitmap_layer_get_layer(charge_layer_black));

  // Create time and date layers
  GRect dummy_frame = { {0, 0}, {0, 0} };
  day_name_layer = bitmap_layer_create(dummy_frame);
  layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));
  for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_MONTH_DIGITS; ++i) {
    month_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(month_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_YEAR_DIGITS; ++i) {
    year_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(year_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_SECONDS_DIGITS; ++i) {
    seconds_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(seconds_digits_layers[i]));
  }

  date_format_layer = bitmap_layer_create(dummy_frame);
  layer_add_child(window_layer, bitmap_layer_get_layer(date_format_layer));

  // Avoids a blank screen on watch start.

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  bluetooth_connection_service_subscribe(handle_bluetooth_connection);
  handle_bluetooth_connection(bluetooth_connection_service_peek());
  battery_state_service_subscribe(handle_battery_state);
  handle_battery_state(battery_state_service_peek());

  // load date format updates the display
  load_date_format();

  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);

  const uint32_t inbound_size = 64;
  const uint32_t outbound_size = 64;
  app_message_open(inbound_size, outbound_size);
}


static void deinit(void) {
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Writing Persistant Data %d", current_date_format);
  persist_write_int(DATE_FORMAT_PKEY, current_date_format);

  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);

  layer_remove_from_parent(bitmap_layer_get_layer(bluetooth_layer));
  bitmap_layer_destroy(bluetooth_layer);
  gbitmap_destroy(bluetooth_image);

  layer_remove_from_parent(bitmap_layer_get_layer(meter_bar_layer));
  bitmap_layer_destroy(meter_bar_layer);
  gbitmap_destroy(meter_bar_image);

  layer_remove_from_parent(bitmap_layer_get_layer(charge_layer_white));
  layer_remove_from_parent(bitmap_layer_get_layer(charge_layer_black));
  bitmap_layer_destroy(charge_layer_white);
  bitmap_layer_destroy(charge_layer_black);

  gbitmap_destroy(charge_image_white);
  gbitmap_destroy(charge_image_black);

  if (time_format_image_loaded) {
  	layer_remove_from_parent(bitmap_layer_get_layer(time_format_layer));
  	bitmap_layer_destroy(time_format_layer);
  	gbitmap_destroy(time_format_image);
  }

  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);

  layer_remove_from_parent(bitmap_layer_get_layer(date_format_layer));
  bitmap_layer_destroy(date_format_layer);
  gbitmap_destroy(date_format_image);

  for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    gbitmap_destroy(date_digits_images[i]);
    bitmap_layer_destroy(date_digits_layers[i]);
  }

  for (int i = 0; i < TOTAL_MONTH_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(month_digits_layers[i]));
    gbitmap_destroy(month_digits_images[i]);
    bitmap_layer_destroy(month_digits_layers[i]);
  }

  for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    bitmap_layer_destroy(time_digits_layers[i]);
  }

  for (int i = 0; i < TOTAL_YEAR_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(year_digits_layers[i]));
    gbitmap_destroy(year_digits_images[i]);
    bitmap_layer_destroy(year_digits_layers[i]);
  }

  for (int i = 0; i < TOTAL_SECONDS_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(seconds_digits_layers[i]));
    gbitmap_destroy(seconds_digits_images[i]);
    bitmap_layer_destroy(seconds_digits_layers[i]);
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

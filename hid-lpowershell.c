
// SPDX-License-Identifier: GPL-2.0-only
/*
 * HID driver for Logitech Powershell gamepad
 *
 */


/**
  This code is based in Bastien Nocera's hid-udraw-ps3 code. It is released under the same GPL license
*/

/**
Logitech powershell device apears as an HID device. This module translates its raw events to joystick events
If a button is pressed, a 12-byte data packet is generated, described as follows
* data[0] = init sequence? Always 1
* data[1..10] = up right down left a b x y l r
* data[11] = "||" button? end of data? Always 0
*/

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include "hid-ids.h"

MODULE_AUTHOR("Marcos Novalbos");
MODULE_DESCRIPTION("Logitech Powershell USB driver");
MODULE_LICENSE("GPL");

#define POWERSHELLID 0xcae2

#define DEVICE_NAME "Logitech powershell gamepad and battery for IPhone"

struct lps_t {
    struct input_dev *joy_input_dev;
    struct hid_device *hdev;
};
/**
 * @brief lps_open Default open function
 * @param dev
 * @return
 */
static int lps_open(struct input_dev *dev)
{
    struct lps_t *lps = input_get_drvdata(dev);
    return hid_hw_open(lps->hdev);
}

/**
 * @brief lps_close Default close function
 * @param dev
 */
static void lps_close(struct input_dev *dev)
{
    struct lps_t *lps = input_get_drvdata(dev);
    hid_hw_close(lps->hdev);
}
/**
 * @brief allocate_and_setup Allocates an input device structure and sets this module open/close functions, ids,etc..
 * @param hdev Hid device
 * @param name Name for this module's device
 * @return
 */
static struct input_dev *allocate_and_setup(struct hid_device *hdev,
        const char *name)
{
    struct input_dev *input_dev;

    input_dev = devm_input_allocate_device(&hdev->dev);
    if (!input_dev)
        return NULL;


    input_dev->name = name;//this device's name
    input_dev->phys = hdev->phys; //phisical address
    input_dev->dev.parent = &hdev->dev;
    input_dev->open = lps_open; //default open/close functions
    input_dev->close = lps_close;
    input_dev->uniq = hdev->uniq;
    input_dev->id.bustype = hdev->bus;
    input_dev->id.vendor  = hdev->vendor; //logitech id
    input_dev->id.product = hdev->product; //powershell id
    input_dev->id.version = hdev->version;
    input_set_drvdata(input_dev, hid_get_drvdata(hdev));

    return input_dev;
}

/**
 * @brief lps_setup_joypad Inits a joypad device structure and setups its events
 * @param lps
 * @param hdev
 * @return true, as it should be
 */
static bool lps_setup_joypad(struct lps_t *lps,
        struct hid_device *hdev)
{
    // Structure for "button" type events,
    struct input_dev *input_dev;
    //Init
    input_dev = allocate_and_setup(hdev, DEVICE_NAME " Joypad");
    if (!input_dev)
        return false;
    //Key type events
    input_dev->evbit[0] = BIT(EV_KEY);

    //Select the gamepad buttons to be used
    set_bit(BTN_DPAD_UP, input_dev->keybit);
    set_bit(BTN_DPAD_DOWN, input_dev->keybit);
    set_bit(BTN_DPAD_LEFT, input_dev->keybit);
    set_bit(BTN_DPAD_RIGHT, input_dev->keybit);
    set_bit(BTN_X, input_dev->keybit);
    set_bit(BTN_Y, input_dev->keybit);
    set_bit(BTN_B, input_dev->keybit);
    set_bit(BTN_A, input_dev->keybit);
    set_bit(BTN_TL, input_dev->keybit);
    set_bit(BTN_TR, input_dev->keybit);
    set_bit(BTN_C, input_dev->keybit);//actually not used

    //Save a reference to this struct
    lps->joy_input_dev = input_dev;
    //Everything OK, as it should be
    return true;
}
/**
 * @brief lps_raw_event Reports events based on the data read from the HID gamepad
 * @param hdev HID device descriptor
 * @param report
 * @param data 12 bytes, described as follows:
 *  data[0] = init sequence?
 *  data[1..10] = up right down left a b x y l r
 *  data[11] = "||" button? end of data? always 0
 * @param len data length: 12 bytes
 * @return
 */
static int lps_raw_event(struct hid_device *hdev, struct hid_report *report,
     u8 *data, int len)
{
    //get our structure
    struct lps_t *lps = hid_get_drvdata(hdev);
    //and report events through our joystick device (/dev/input/jsXX)
    input_report_key(lps->joy_input_dev, BTN_DPAD_UP,data[1] );
    input_report_key(lps->joy_input_dev, BTN_DPAD_RIGHT,data[2] );
    input_report_key(lps->joy_input_dev, BTN_DPAD_DOWN,data[3] );
    input_report_key(lps->joy_input_dev, BTN_DPAD_LEFT,data[4] );
    input_report_key(lps->joy_input_dev, BTN_A,data[5] );
    input_report_key(lps->joy_input_dev, BTN_B,data[6] );
    input_report_key(lps->joy_input_dev, BTN_X,data[7] );
    input_report_key(lps->joy_input_dev, BTN_Y,data[8] );
    input_report_key(lps->joy_input_dev, BTN_TL,data[9] );
    input_report_key(lps->joy_input_dev, BTN_TR,data[10] );    
    input_report_key(lps->joy_input_dev, BTN_C,data[11] );
    //Sync before return
    input_sync(lps->joy_input_dev);

    return 0;
}

/**
 * First module function invoked. Initializes the module
 * @param hdev Pointer to the structure of the connected HID device
 * @param id HID identifier, not used
 * **/

static int lps_probe(struct hid_device *hdev, const struct hid_device_id *id)
{

    //structure that holds this module "persistent" data. Will keep references to the initialized kernel devices used
    struct lps_t *lps;
    int ret;
    //Allocate memory for our module data
    lps = devm_kzalloc(&hdev->dev, sizeof(struct lps_t), GFP_KERNEL);
    if (!lps)
        return -ENOMEM;
    //keep a reference to the HID device
    lps->hdev = hdev;
    //Link our structure inside the kernel, so it can be retrieved later
     hid_set_drvdata(hdev, lps);
    //check errors
     ret = hid_parse(hdev);
     if (ret) {
         hid_err(hdev, "LPS: parse failed\n");
         return ret;
     }
     //setup our events and check errors
     if (!lps_setup_joypad(lps, hdev)) {
         hid_err(hdev, "LPS: could not allocate interfaces\n");
         return -ENOMEM;
     }

      //register the new joystick event module
     ret = input_register_device(lps->joy_input_dev);
     if (ret) {
         hid_err(hdev, "LPS: failed to register interfaces\n");
         return ret;
     }

     //and let the HID hardware start, this module will take care of the events sent by the device
     ret = hid_hw_start(hdev, HID_CONNECT_HIDRAW | HID_CONNECT_DRIVER);
     if (ret) {
         hid_err(hdev, "LPS: hw start failed\n");
         return ret;
     }
     return 0;
}

// IDS accepted by this module
static const struct hid_device_id lps_devices[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_LOGITECH, POWERSHELLID) },
    { }
};
MODULE_DEVICE_TABLE(hid, lps_devices);

//kernel module structure
static struct hid_driver lps_driver = {
    .name = "hid-lpowershell",
    .id_table = lps_devices, //device ids managed by this module
    .raw_event = lps_raw_event, //receives device' s data
    .probe = lps_probe,//inits this module
};
//notify our module
module_hid_driver(lps_driver);



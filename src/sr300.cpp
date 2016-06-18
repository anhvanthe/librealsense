// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <algorithm>

#include "image.h"
#include "sr300.h"
#include "sr300-private.h"

namespace rsimpl
{
    const std::vector <std::pair<rs_option, char>> eu_SR300_depth_controls = {{rs_option::RS_OPTION_F200_LASER_POWER,          0x01},
                                                                              {rs_option::RS_OPTION_F200_ACCURACY,             0x02},
                                                                              {rs_option::RS_OPTION_F200_MOTION_RANGE,         0x03},
                                                                              {rs_option::RS_OPTION_F200_FILTER_OPTION,        0x05},
                                                                              {rs_option::RS_OPTION_F200_CONFIDENCE_THRESHOLD, 0x06}};

    static const cam_mode sr300_color_modes[] = {
        {{1920, 1080}, {5,15,30}},
        {{1280,  720}, {5,15,30,60}},
        {{ 960,  540}, {5,15,30,60}},
        {{ 848,  480}, {5,15,30,60}},
        {{ 640,  480}, {5,15,30,60}},
        {{ 640,  360}, {5,15,30,60}},
        {{ 424,  240}, {5,15,30,60}},
        {{ 320,  240}, {5,15,30,60}},
        {{ 320,  180}, {5,15,30,60}}
    };

    static const cam_mode sr300_depth_modes[] = {
        {{640, 480}, {5,15,30,60}}, 
        {{640, 240}, {5,15,30,60,110}}
    };

    static const cam_mode sr300_ir_only_modes[] = {
        {{640, 480}, {30,60,120,200}}      
    };    

    static static_device_info get_sr300_info(std::shared_ptr<uvc::device> device, const iv::camera_calib_params & c)
    {
        LOG_INFO("Connecting to Intel RealSense SR300");

        static_device_info info;
        info.name = {"Intel RealSense SR300"};
        
        // Color modes on subdevice 0
        info.stream_subdevices[RS_STREAM_COLOR] = 0;
        for(auto & m : sr300_color_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({0, m.dims, pf_yuy2, fps, MakeColorIntrinsics(c, m.dims), {}, {0}});
            }
        }

        // Depth and IR modes on subdevice 1
        info.stream_subdevices[RS_STREAM_DEPTH] = 1;
        info.stream_subdevices[RS_STREAM_INFRARED] = 1;
        for(auto & m : sr300_ir_only_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.dims, pf_sr300_invi, fps, MakeDepthIntrinsics(c, m.dims), {}, {0}});             
            }
        }
        for(auto & m : sr300_depth_modes)
        {
            for(auto fps : m.fps)
            {
                info.subdevice_modes.push_back({1, m.dims, pf_invz, fps, MakeDepthIntrinsics(c, m.dims), {}, {0}});       
                info.subdevice_modes.push_back({1, m.dims, pf_sr300_inzi, fps, MakeDepthIntrinsics(c, m.dims), {}, {0}});
            }
        }

        for(int i=0; i<RS_PRESET_COUNT; ++i)
        {
            info.presets[RS_STREAM_COLOR   ][i] = {true, 640, 480, RS_FORMAT_RGB8, 60};
            info.presets[RS_STREAM_DEPTH   ][i] = {true, 640, 480, RS_FORMAT_Z16, 60};
            info.presets[RS_STREAM_INFRARED][i] = {true, 640, 480, RS_FORMAT_Y16, 60};
        }

        info.options = {
            {RS_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE, 0.0,              2.0,                                       1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_ENABLE_LASER,               0.0,              1.0,                                       1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_MIN_MOTION_VERSUS_RANGE,    (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_MAX_MOTION_VERSUS_RANGE,    (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_START_MOTION_VERSUS_RANGE,  (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_MIN_LASER,                  (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_MAX_LASER,                  (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_START_LASER,                (double)SHRT_MIN, (double)SHRT_MAX,                          1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_UPPER_THRESHOLD,            0.0,              (double)USHRT_MAX,                         1.0,   -1.0},
            {RS_OPTION_SR300_AUTO_RANGE_LOWER_THRESHOLD,            0.0,              (double)USHRT_MAX,                         1.0,   -1.0},
            {RS_OPTION_SR300_WAKEUP_DEV_PHASE1_PERIOD,              0.0,              (double)USHRT_MAX,                         1.0,   -1.0},
            {RS_OPTION_SR300_WAKEUP_DEV_PHASE1_FPS,                 0.0,              ((double)sr300::e_suspend_fps::eFPS_MAX) - 1,      1.0, -1.0},
            {RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD,              0.0,              (double)USHRT_MAX, 1.0, -1.0},
            {RS_OPTION_SR300_WAKEUP_DEV_PHASE2_FPS,                 0.0,              ((double)sr300::e_suspend_fps::eFPS_MAX) - 1,      1.0, -1.0},
            {RS_OPTION_SR300_WAKEUP_DEV_RESET,                      0.0,              0.0,               1.0, -1.0},
            {RS_OPTION_SR300_WAKE_ON_USB_REASON,                    0.0,              (double)sr300::wakeonusb_reason::eMaxWakeOnReason, 1.0, -1.0},
            {RS_OPTION_SR300_WAKE_ON_USB_CONFIDENCE,                0.0,              100.,                                              1.0, -1.0}  // Percentage
        };

        update_supported_options(*device.get(), iv::depth_xu, eu_SR300_depth_controls, info.options);

        rsimpl::pose depth_to_color = {transpose((const float3x3 &)c.Rt), (const float3 &)c.Tt * 0.001f}; // convert mm to m
        info.stream_poses[RS_STREAM_DEPTH] = info.stream_poses[RS_STREAM_INFRARED] = inverse(depth_to_color);
        info.stream_poses[RS_STREAM_COLOR] = {{{1,0,0},{0,1,0},{0,0,1}}, {0,0,0}};

        info.nominal_depth_scale = (c.Rmax / 0xFFFF) * 0.001f; // convert mm to m
        info.num_libuvc_transfer_buffers = 1;
        return info;
    }

    sr300_camera::sr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const iv::camera_calib_params & calib) :
        iv_camera(device, info, calib)
    {
        // TODO Evgeni
        //// These settings come from the "Common" preset. There is no actual way to read the current values off the device.
        //arr.enableMvR = 1;
        //arr.enableLaser = 1;
        //arr.minMvR = 180;
        //arr.maxMvR = 605;
        //arr.startMvR = 303;
        //arr.minLaser = 2;
        //arr.maxLaser = 16;
        //arr.startLaser = -1;
        //arr.ARUpperTh = 1250;
        //arr.ARLowerTh = 650;
    }

    sr300_camera::~sr300_camera()
    {
    }

    void sr300_camera::on_before_start(const std::vector<subdevice_mode_selection> & selected_modes)
    {

    }
    
    rs_stream sr300_camera::select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes)
    {
        int fps[RS_STREAM_NATIVE_COUNT] = {}, max_fps = 0;
        for(const auto & m : selected_modes)
        {
            for(const auto & output : m.get_outputs())
            {
                fps[output.first] = m.mode.fps;
                max_fps = std::max(max_fps, m.mode.fps);
            }
        }

        // Prefer to sync on depth or infrared, but select the stream running at the fastest framerate
        for (auto s : { RS_STREAM_DEPTH, RS_STREAM_INFRARED2, RS_STREAM_INFRARED, RS_STREAM_COLOR })
        {
            if(fps[s] == max_fps) return s;
        }
        return RS_STREAM_DEPTH;
    }

    void sr300_camera::set_options(const rs_option options[], size_t count, const double values[])
    {
        std::vector<rs_option>  base_opt;
        std::vector<double>     base_opt_val;

        auto arr_writer = make_struct_interface<iv::cam_auto_range_request>([this]() { return arr; }, [this](iv::cam_auto_range_request r) {
            iv::set_auto_range(get_device(), usbMutex, r.enableMvR, r.minMvR, r.maxMvR, r.startMvR, r.enableLaser, r.minLaser, r.maxLaser, r.startLaser, r.ARUpperTh, r.ARLowerTh);
            arr = r;
        });

        auto arr_wakeup_dev_writer = make_struct_interface<sr300::wakeup_dev_params>([this]() { return arr_wakeup_dev_param; }, [this](sr300::wakeup_dev_params param) {
            sr300::set_wakeup_device(get_device(), usbMutex, param.phase1Period, (uint32_t)param.phase1FPS, param.phase2Period, (uint32_t)param.phase2FPS);
            arr_wakeup_dev_param = param;
        });

        for(size_t i=0; i<count; ++i)
        {
            if(uvc::is_pu_control(options[i]))
            {
                uvc::set_pu_control_with_retry(get_device(), 0, options[i], static_cast<int>(values[i]));
                continue;
            }

            switch(options[i])
            {
            case RS_OPTION_SR300_WAKEUP_DEV_RESET:    sr300::reset_wakeup_device(get_device(), usbMutex); break;

            case RS_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE: arr_writer.set(&iv::cam_auto_range_request::enableMvR, values[i]); break; 
            case RS_OPTION_SR300_AUTO_RANGE_ENABLE_LASER:               arr_writer.set(&iv::cam_auto_range_request::enableLaser, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_MIN_MOTION_VERSUS_RANGE:    arr_writer.set(&iv::cam_auto_range_request::minMvR, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_MAX_MOTION_VERSUS_RANGE:    arr_writer.set(&iv::cam_auto_range_request::maxMvR, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_START_MOTION_VERSUS_RANGE:  arr_writer.set(&iv::cam_auto_range_request::startMvR, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_MIN_LASER:                  arr_writer.set(&iv::cam_auto_range_request::minLaser, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_MAX_LASER:                  arr_writer.set(&iv::cam_auto_range_request::maxLaser, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_START_LASER:                arr_writer.set(&iv::cam_auto_range_request::startLaser, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_UPPER_THRESHOLD:            arr_writer.set(&iv::cam_auto_range_request::ARUpperTh, values[i]); break;
            case RS_OPTION_SR300_AUTO_RANGE_LOWER_THRESHOLD:            arr_writer.set(&iv::cam_auto_range_request::ARLowerTh, values[i]); break;

            case RS_OPTION_SR300_WAKEUP_DEV_PHASE1_PERIOD:              arr_wakeup_dev_writer.set(&sr300::wakeup_dev_params::phase1Period, values[i]); break;
            case RS_OPTION_SR300_WAKEUP_DEV_PHASE1_FPS:                 arr_wakeup_dev_writer.set(&sr300::wakeup_dev_params::phase1FPS, (int)values[i]); break;
            case RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD:              arr_wakeup_dev_writer.set(&sr300::wakeup_dev_params::phase2Period, values[i]); break;
            case RS_OPTION_SR300_WAKEUP_DEV_PHASE2_FPS:                 arr_wakeup_dev_writer.set(&sr300::wakeup_dev_params::phase2FPS, (int)values[i]); break;

            case RS_OPTION_SR300_WAKE_ON_USB_REASON:                    LOG_WARNING("Read-only property: " << options[i] << " on " << get_name()); break;
            case RS_OPTION_SR300_WAKE_ON_USB_CONFIDENCE:                LOG_WARNING("Read-only property: " << options[i] << " on " << get_name()); break;

                // Default will be handled by parent implementation
            default: base_opt.push_back(options[i]); base_opt_val.push_back(values[i]); break;
            }
        }

        arr_writer.commit();
        arr_wakeup_dev_writer.commit();

        //Handle common options
        if (base_opt.size())
            iv_camera::set_options(base_opt.data(), base_opt.size(), base_opt_val.data());
    }

    void sr300_camera::get_options(const rs_option options[], size_t count, double values[])
    {

        std::vector<rs_option>  base_opt;
        std::vector<size_t>     base_opt_index;
        std::vector<double>     base_opt_val;

        auto arr_reader = make_struct_interface<iv::cam_auto_range_request>([this]() { return arr; }, [this](iv::cam_auto_range_request) {});
        auto wake_on_usb_reader = make_struct_interface<sr300::wakeup_dev_params>([this]()
        { return arr_wakeup_dev_param; },[]() { throw std::logic_error("Read-only operation"); });


        // Acquire SR300-specific options first
        for(int i=0; i<count; ++i)
        {
            LOG_INFO("Reading option " << options[i]);

            if(uvc::is_pu_control(options[i]))
            {
                values[i] = uvc::get_pu_control(get_device(), 0, options[i]);
                continue;
            }

            uint8_t val=0;
            switch(options[i])
            {
            case RS_OPTION_SR300_AUTO_RANGE_ENABLE_MOTION_VERSUS_RANGE: values[i] = arr_reader.get(&iv::cam_auto_range_request::enableMvR); break; 
            case RS_OPTION_SR300_AUTO_RANGE_ENABLE_LASER:               values[i] = arr_reader.get(&iv::cam_auto_range_request::enableLaser); break;
            case RS_OPTION_SR300_AUTO_RANGE_MIN_MOTION_VERSUS_RANGE:    values[i] = arr_reader.get(&iv::cam_auto_range_request::minMvR); break;
            case RS_OPTION_SR300_AUTO_RANGE_MAX_MOTION_VERSUS_RANGE:    values[i] = arr_reader.get(&iv::cam_auto_range_request::maxMvR); break;
            case RS_OPTION_SR300_AUTO_RANGE_START_MOTION_VERSUS_RANGE:  values[i] = arr_reader.get(&iv::cam_auto_range_request::startMvR); break;
            case RS_OPTION_SR300_AUTO_RANGE_MIN_LASER:                  values[i] = arr_reader.get(&iv::cam_auto_range_request::minLaser); break;
            case RS_OPTION_SR300_AUTO_RANGE_MAX_LASER:                  values[i] = arr_reader.get(&iv::cam_auto_range_request::maxLaser); break;
            case RS_OPTION_SR300_AUTO_RANGE_START_LASER:                values[i] = arr_reader.get(&iv::cam_auto_range_request::startLaser); break;
            case RS_OPTION_SR300_AUTO_RANGE_UPPER_THRESHOLD:            values[i] = arr_reader.get(&iv::cam_auto_range_request::ARUpperTh); break;
            case RS_OPTION_SR300_AUTO_RANGE_LOWER_THRESHOLD:            values[i] = arr_reader.get(&iv::cam_auto_range_request::ARLowerTh); break;

            case RS_OPTION_SR300_WAKEUP_DEV_PHASE1_PERIOD:              values[i] = wake_on_usb_reader.get(&sr300::wakeup_dev_params::phase1Period); break;
            case RS_OPTION_SR300_WAKEUP_DEV_PHASE1_FPS:                 values[i] = wake_on_usb_reader.get(&sr300::wakeup_dev_params::phase1FPS); break;
            case RS_OPTION_SR300_WAKEUP_DEV_PHASE2_PERIOD:              values[i] = wake_on_usb_reader.get(&sr300::wakeup_dev_params::phase2Period); break;
            case RS_OPTION_SR300_WAKEUP_DEV_PHASE2_FPS:                 values[i] = wake_on_usb_reader.get(&sr300::wakeup_dev_params::phase2FPS); break;

            case RS_OPTION_SR300_WAKE_ON_USB_REASON:        sr300::get_wakeup_reason(get_device(), usbMutex, val);		values[i] = val; break;
            case RS_OPTION_SR300_WAKE_ON_USB_CONFIDENCE:    sr300::get_wakeup_confidence(get_device(), usbMutex, val);	values[i] = val; break;

                // Default will be handled by parent implementation
            default: base_opt.push_back(options[i]); base_opt_index.push_back(i);  break;
            }
        }

        //Then retrieve the common options
        if (base_opt.size())
        {
            base_opt_val.resize(base_opt.size());
            iv_camera::get_options(base_opt.data(), base_opt.size(), base_opt_val.data());
        }

        // Merge the local data with values obtained by base class
        for (auto i : base_opt_index)
            values[i] = base_opt_val[i];
    }

    std::shared_ptr<rs_device> make_sr300_device(std::shared_ptr<uvc::device> device)
    {
        std::timed_mutex mutex;
        iv::claim_ivcam_interface(*device);
        auto calib = sr300::read_sr300_calibration(*device, mutex);
        iv::enable_timestamp(*device, mutex, true, true);

        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_BACKLIGHT_COMPENSATION, 0);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_BRIGHTNESS, 0);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_CONTRAST, 50);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_GAMMA, 300);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_HUE, 0);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_SATURATION, 64);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_SHARPNESS, 50);
        uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_GAIN, 64);
        //uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_WHITE_BALANCE, 4600); // auto
        //uvc::set_pu_control_with_retry(*device, 0, rs_option::RS_OPTION_COLOR_EXPOSURE, -6); // auto

        auto info = get_sr300_info(device, calib);

        iv::get_module_serial_string(*device, mutex, info.serial, 132);
        iv::get_firmware_version_string(*device, mutex, info.firmware_version);

        info.capabilities_vector.push_back(RS_CAPABILITIES_COLOR);
        info.capabilities_vector.push_back(RS_CAPABILITIES_DEPTH);
        info.capabilities_vector.push_back(RS_CAPABILITIES_INFRARED);

        return std::make_shared<sr300_camera>(device, info, calib);
    }

} // namespace rsimpl::sr300
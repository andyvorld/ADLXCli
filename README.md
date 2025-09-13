# ADLXCli
A cli tool to control tuning settings of modern AMD GPUs; validated on a 7900XTX

- GPU Tuning
    - Min/Max Frequency
    - Voltage
- VRAM Tuning
    - Max Frequency
- Fan Tuning
    - Zero RPM toggle
    - Fan Curve
- Power Tuning
    - Power Limit

## Warning
ADLXCli uses the official ADLX api (https://github.com/GPUOpen-LibrariesAndSDKs/ADLX), and should replicate the Adrenaline application closely.

As with all overclocking, user beware with regards to system stability and lifespan of their own hardware.

# TODO
- [ ] Remove hard coded profile values
- [ ] Interact via json input
- [ ] Dump current profile to json

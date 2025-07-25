def user_script1():
    print("Running user_script1")
    daq.setBias(1300)
    daq.startAcquisition()
    daq.stopAcquisition()

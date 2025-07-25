def user_script2():
    print("Running user_script2")
    daq.setBias(900)
    daq.setBias(1500)
    daq.startAcquisition()
    daq.stopAcquisition()

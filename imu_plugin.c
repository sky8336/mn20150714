
//IMU plugin jieshou zhuanhuan:

OSVR_ReturnCode update() {
	if (shuttingdown) 
		return OSVR_RETURN_SUCCESS;
	if (!connected) {
		setup_port_forward();
		connected = connect_to_server();
		if (!connected) // try another time
			return OSVR_RETURN_SUCCESS;
	}

	memset(buffer, 0, BUFSIZE);
	int ret = recv(sockfd, buffer, BUFSIZE - 1, 0);
	if (ret <= 0) {
		// Receive error or socket closed. This could be because of a USB 3 problem specific to some devices.
		// The OSVR render manager changes will take care of re-establishing the USB link, but the adb forward
		// needs to be setup again. Try doing that here and skip this update, hopefully we should be able to
		// reconnect in the next attempt.
		disconnect();
		return OSVR_RETURN_SUCCESS;
	}

	std::vector<std::string> elems;
	split(buffer, ',', elems);
	if (elems.size() < 4) {
		// Invalid
		std::cout << "VRUSB PLUGIN: Invalid input received - " << buffer << std::endl;
		return OSVR_RETURN_SUCCESS;
	}

	OSVR_OrientationState orientation;
	osvrQuatSetIdentity(&orientation);
	osvrQuatSetW(&orientation, atof(elems.at(0).c_str()));
	osvrQuatSetX(&orientation, atof(elems.at(1).c_str()));
	osvrQuatSetY(&orientation, atof(elems.at(2).c_str()));
	osvrQuatSetZ(&orientation, atof(elems.at(3).c_str()));
	//std::cout << "IMU: " << orientation.data[0] << "," << orientation.data[1] << "," << orientation.data[2] << "," << orientation.data[3] << std::endl;
	osvrDeviceTrackerSendOrientation(m_dev, m_tracker, &orientation, 0);
	return OSVR_RETURN_SUCCESS;
}

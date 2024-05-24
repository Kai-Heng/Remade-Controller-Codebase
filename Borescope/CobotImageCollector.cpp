#include "ImageCollector.h"

CobotImageCollector::CobotImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages) :
	CaptureImageCollector(camera, numImages),
	initialTriggerSelector(camera.TriggerSelector.GetValue()),
	initialTriggerMode(camera.TriggerMode.GetValue()),
	initialTriggerSource(camera.TriggerSource.GetValue())
{
	CaptureTimeout = 60000;
	camera.LineSelector.SetValue(LineSelector_Line1);
	camera.LineMode.SetValue(LineMode_Input);
	camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
	camera.TriggerMode.SetValue(TriggerMode_On);
	camera.TriggerSource.SetValue(TriggerSource_Line1);
	camera.TriggerActivation.SetValue(TriggerActivation_RisingEdge);
	//camera.TriggerDelay.SetValue(2500000);
	camera.LineSelector.SetValue(LineSelector_Line2);
	camera.LineMode.SetValue(LineMode_Output);
	camera.LineSource.SetValue(LineSource_UserOutput1);
	camera.UserOutputSelector.SetValue(UserOutputSelector_UserOutput1);
	/*camera.LineSelector.SetValue(LineSelector_Line2);
	camera.LineSource.SetValue(LineSource_UserOutput1);
	camera.UserOutputSelector.SetValue(UserOutputSelector_UserOutput1);
	camera.UserOutputValue.SetValue(true);*/
}

CobotImageCollector::~CobotImageCollector() {
	camera.TriggerSelector.SetValue(initialTriggerSelector);
	camera.TriggerMode.SetValue(initialTriggerMode);
	camera.TriggerSource.SetValue(initialTriggerSource);
}

vector<Image> CobotImageCollector::GetImages() {
	vector<Image> images;
	camera.MaxNumBuffer = MaxNumBuffer;

	camera.StartGrabbing(numImages);

	while (camera.IsGrabbing())
	{
		cout << "Waiting for cobot" << endl;
		CGrabResultPtr ptrGrabResult;
		camera.RetrieveResult(CaptureTimeout, ptrGrabResult);

		if (ptrGrabResult->GrabSucceeded()) {
			images.push_back(Image::from(ptrGrabResult));
			camera.UserOutputValueAll.SetValue(true);
			cout << "Image " << images.size() - 1 << " captured" << endl;
			camera.UserOutputValueAll.SetValue(false);
		}
		else cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
	}

	return images;
}
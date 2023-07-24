#include "ImageCollector.h"

LineScanImageCollector::LineScanImageCollector(CBaslerUniversalInstantCamera& camera, const size_t numImages) :
	CaptureImageCollector(camera, numImages),
	initialTriggerSelector(camera.TriggerSelector.GetValue()),
	initialTriggerMode(camera.TriggerMode.GetValue()),
	initialTriggerSource(camera.TriggerSource.GetValue())
{
	CaptureTimeout = 60000;

	camera.AcquisitionMode.SetValue(AcquisitionMode_Continuous);

	camera.TriggerSelector.SetValue(TriggerSelector_AcquisitionStart);
	camera.TriggerMode.SetValue(TriggerMode_Off);

	camera.TriggerSelector.SetValue(TriggerSelector_FrameStart);
	camera.TriggerMode.SetValue(TriggerMode_Off);
	//camera.TriggerMode.SetValue(TriggerMode_On);
	//camera.TriggerSource.SetValue(TriggerSource_Software);

	camera.TriggerSelector.SetValue(TriggerSelector_LineStart);
	camera.TriggerMode.SetValue(TriggerMode_Off);
}

LineScanImageCollector::~LineScanImageCollector() {
	camera.TriggerSelector.SetValue(initialTriggerSelector);
	camera.TriggerMode.SetValue(initialTriggerMode);
	camera.TriggerSource.SetValue(initialTriggerSource);
}

vector<Image> LineScanImageCollector::GetImages() {
	vector<Image> images;
	camera.MaxNumBuffer = MaxNumBuffer;

	camera.StartGrabbing(numImages);

	while (camera.IsGrabbing())
	{
		//TriggerCapture(camera);
		CGrabResultPtr ptrGrabResult;
		camera.RetrieveResult(CaptureTimeout, ptrGrabResult);

		if (ptrGrabResult->GrabSucceeded()) images.push_back(Image::from(ptrGrabResult));
		else cout << "Error: " << ptrGrabResult->GetErrorCode() << " " << ptrGrabResult->GetErrorDescription() << endl;
	}

	return images;
}

void LineScanImageCollector::TriggerCapture(CBaslerUniversalInstantCamera& camera) {
	cout << "Ready" << endl;
	cin.get();
	cout << "Capturing..." << endl;
	camera.TriggerSoftware.Execute();
}
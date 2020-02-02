//---------------------------------------------------------------------------------------------------------------------
//  DVSAL
//---------------------------------------------------------------------------------------------------------------------
//  Copyright 2019 - Marco Montes Grova (a.k.a. marrcogrova) 
//---------------------------------------------------------------------------------------------------------------------
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
//  and associated documentation files (the "Software"), to deal in the Software without restriction, 
//  including without limitation the rights to use, copy, modify, merge, publish, distribute, 
//  sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is 
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all copies or substantial 
//  portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING 
//  BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
//  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES 
//  OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN 
//  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//---------------------------------------------------------------------------------------------------------------------

#include "dvsal/blocks/BlockDvImageVisualizer.h"

namespace dvsal{

    BlockDvImageVisualizer::BlockDvImageVisualizer(){

        mapper_ = vtkSmartPointer<vtkImageMapper>::New();
        
        image_ = vtkSmartPointer<vtkActor2D>::New();
        image_->SetMapper(mapper_);

        renderer_ = vtkSmartPointer<vtkRenderer>::New();
        renderer_->AddActor(image_);

        window_ = vtkSmartPointer<vtkRenderWindow>::New();
        window_->AddRenderer(renderer_);

        createPolicy({{"DVS Events","v_event"}});

        registerCallback({"DVS Events"}, 
                [&](flow::DataFlow  _data){
                        if(idle_){
                            idle_ = false;  
                                        
                            dv::EventStore events = _data.get<dv::EventStore>("DVS Events");
                            cv::Mat fakeImage = convertEventsToCVMat(events);
                            if(fakeImage.rows != 0){
                                auto vtkImage = convertCVMatToVtkImageData(fakeImage, true);
                                mapper_->SetInputData(vtkImage);
                                mapper_->SetColorWindow(255); // width of the color range to map to
                                mapper_->SetColorLevel(127.5); // center of the color range to map to

                                int imageSize[3];
                                vtkImage->GetDimensions(imageSize);
                                window_->SetSize(imageSize[0], imageSize[1]);

                                window_->Render();
                            }
                            idle_ = true;
                        }
                    }
                );
    }

    cv::Mat BlockDvImageVisualizer::convertEventsToCVMat(dv::EventStore _events){
        cv::Mat image = cv::Mat::zeros(cv::Size(240,180), CV_8UC3);
        for (const auto &event : _events) {
            if (event.polarity())
                image.at<cv::Vec3b>(event.y(), event.x()) = cv::Vec3b(0,0,255);
            else
                image.at<cv::Vec3b>(event.y(), event.x()) = cv::Vec3b(0,255,0);
        }

        return image;
    }

    vtkSmartPointer<vtkImageData> BlockDvImageVisualizer::convertCVMatToVtkImageData(const cv::Mat &sourceCVImage, bool flipOverXAxis) {
        cv::Mat sourceImage;
        if(sourceCVImage.channels() == 1){
            cv::cvtColor(sourceCVImage, sourceImage, cv::ColorConversionCodes::COLOR_GRAY2RGB);
        }else{
            cv::cvtColor(sourceCVImage, sourceImage, cv::ColorConversionCodes::COLOR_BGR2RGB);
        }
        
        vtkSmartPointer<vtkImageData> outputVtkImage = vtkSmartPointer<vtkImageData>::New();
        double spacing[3] = {1, 1, 1};
        double origin[3] = {0, 0, 0};
        int extent[6] = {0, sourceImage.cols - 1, 0, sourceImage.rows - 1, 0, 0};
        auto numOfChannels = sourceImage.channels();
        outputVtkImage->SetSpacing(spacing);
        outputVtkImage->SetOrigin(origin);
        outputVtkImage->SetExtent(extent);
        outputVtkImage->SetDimensions(sourceImage.cols, sourceImage.rows, 1);
        outputVtkImage->AllocateScalars(VTK_UNSIGNED_CHAR, numOfChannels);

        cv::Mat tempCVImage;
        if (flipOverXAxis) { // Normaly you should flip the image!
            cv::flip(sourceImage, tempCVImage, 0);
        }
        else {
            tempCVImage = sourceImage;
        }
        
        unsigned char* dptr = reinterpret_cast<unsigned char*>(outputVtkImage->GetScalarPointer());
        mempcpy(dptr, tempCVImage.data, sourceImage.cols*sourceImage.rows*3);

        outputVtkImage->Modified();

        return outputVtkImage;
    }      

}
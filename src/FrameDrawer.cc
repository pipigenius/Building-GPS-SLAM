/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/

#include "FrameDrawer.h"
#include "Tracking.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include<mutex>

namespace ORB_SLAM2
{

FrameDrawer::FrameDrawer(Map* pMap):mpMap(pMap)
{
    mState=Tracking::SYSTEM_NOT_READY;
    mIm = cv::Mat(720, 640,CV_8UC3, cv::Scalar(0,0,0));
    mImask = cv::Mat(360, 640,CV_8UC3, cv::Scalar(0,0,0));
    logo = imread("/Users/vision/Downloads/luyu/project/ORB_SLAM_xcode_graz/Examples/logo.png");
}

cv::Mat FrameDrawer::DrawFrame()
{
    cv::Mat im, imask, now_caTwc;
    vector<cv::KeyPoint> vIniKeys; // Initialization: KeyPoints in reference frame
    vector<int> vMatches; // Initialization: correspondeces with reference keypoints
    vector<cv::KeyPoint> vCurrentKeys; // KeyPoints in current frame
    vector<bool> vbVO, vbMap; // Tracked MapPoints in current frame
    int state; // Tracking state
    //Copy variables within scoped mutex
    {
        unique_lock<mutex> lock(mMutex);
        state=mState;
        if(mState==Tracking::SYSTEM_NOT_READY)
            mState=Tracking::NO_IMAGES_YET;

        mIm.copyTo(im);
        mImask.copyTo(imask);
        ca_Twc.copyTo(now_caTwc);
        if(mState==Tracking::NOT_INITIALIZED)
        {
            vCurrentKeys = mvCurrentKeys;
            vIniKeys = mvIniKeys;
            vMatches = mvIniMatches;
        }
        else if(mState==Tracking::OK)
        {
            vCurrentKeys = mvCurrentKeys;
            vbVO = mvbVO;
            vbMap = mvbMap;
        }
        else if(mState==Tracking::LOST)
        {
            vCurrentKeys = mvCurrentKeys;
        }
    } // destroy scoped mutex -> release mutex

    //this should be always true
    if(im.channels()<3) {
        cvtColor(im,im,CV_GRAY2BGR);
        cvtColor(imask,imask,CV_GRAY2BGR);
    }


    if(imask.channels()<3)
    {
        cvtColor(im,im,CV_RGBA2RGB);
        cvtColor(imask,imask,CV_GRAY2RGB);
    }


    //Draw
    if(state==Tracking::NOT_INITIALIZED) //INITIALIZING
    {
        for(unsigned int i=0; i<vMatches.size(); i++)
        {
            if(vMatches[i]>=0)
            {
//                cv::line(im,vIniKeys[i].pt,vCurrentKeys[vMatches[i]].pt,
//                        cv::Scalar(0,255,0));
            }
        }        
    }
    else if(state==Tracking::OK) //TRACKING
    {
        mnTracked=0;
        mnTrackedVO=0;
        const float r = 5;
        const int n = vCurrentKeys.size();
        for(int i=0;i<n;i++)
        {
            if(vbVO[i] || vbMap[i])
            {
                cv::Point2f pt1,pt2;
                pt1.x=vCurrentKeys[i].pt.x-r;
                pt1.y=vCurrentKeys[i].pt.y-r;
                pt2.x=vCurrentKeys[i].pt.x+r;
                pt2.y=vCurrentKeys[i].pt.y+r;

                // This is a match to a MapPoint in the map
                if(vbMap[i])
                {
                    cv::rectangle(im,pt1,pt2,cv::Scalar(0,255,0));
                    cv::circle(im,vCurrentKeys[i].pt,2,cv::Scalar(0,255,0),-1);
                    mnTracked++;
                }
                else // This is match to a "visual odometry" MapPoint created in the last frame
                {
                    cv::rectangle(im,pt1,pt2,cv::Scalar(255,0,0));
                    cv::circle(im,vCurrentKeys[i].pt,2,cv::Scalar(255,0,0),-1);
                    mnTrackedVO++;
                }
            }
        }
 /*
        //sensor
        for(int i=0;i<imask.rows;i++){
            for(int j=0;j<imask.cols;j++){
                if(imask.at<Vec3b>(i,j)[0]<255) {
                    imask.at<Vec3b>(i,j) = cv::Vec3b(125,125,0) + 0.5 * im.at<Vec3b>(i,j);
                }else{
                    imask.at<Vec3b>(i,j) = im.at<Vec3b>(i,j);
                }
            }
        }
        
        //ar
        //input ab_Twc, ca_Twc, mpbuildings,
        double fov;//=atan(imgWidth/2*fx)
        fov = 2*atan(640/(2*618.126667));
        fov = Config::r2a(fov);

        cv::Point2f camera_pt;
        cv::Mat Ow = now_caTwc.rowRange(0, 3).col(3);
        camera_pt.x = Ow.at<float>(0);
        camera_pt.y = Ow.at<float>(1);
        float cur_yaw ;
        cur_yaw = rela_yaw;

        Config2::PointSet all_seenbuilding;
        all_seenbuilding = Config2::find_seenfacade(camera_pt, cur_yaw, fov, *mpbuildings);
        
        cv::Mat poly = im.clone();
        for (ORB_SLAM2::Config2::PointSet::iterator it=all_seenbuilding.begin(); it!=all_seenbuilding.end(); ++it)
        {
            cv::Point2i seen;
            seen = *it;
            vector<cv::Point3f> eachfacade = (*mpbuildings)[seen.x][seen.y];
            
            cv::Mat ca_Tcw;
            ca_Tcw = now_caTwc.inv();
            cv::Point2i pts[4];
            //our pose
            pts[0] = Config::Fn_Projection_pt(eachfacade[0], ca_Tcw);
            pts[1] = Config::Fn_Projection_pt(eachfacade[1], ca_Tcw);
            pts[2] = Config::Fn_Projection_pt(eachfacade[3], ca_Tcw);
            pts[3] = Config::Fn_Projection_pt(eachfacade[4], ca_Tcw);
            cv::fillConvexPoly(poly, pts, 4, cv::Scalar(0,255,255));

        }
        cv::addWeighted(im, 0.6, poly, 0.4, 1, im);*/
    }

//    Config::mergeImg(im, im, imask);
    cv::Mat imWithInfo;
    DrawTextInfo(im, state, imWithInfo);

    return imWithInfo;
}


void FrameDrawer::DrawTextInfo(cv::Mat &im, int nState, cv::Mat &imText)
{
    stringstream s;
    if(nState==Tracking::NO_IMAGES_YET)
        s << " WAITING FOR IMAGES";
    else if(nState==Tracking::NOT_INITIALIZED)
        s << " TRYING TO INITIALIZE ";
    else if(nState==Tracking::OK)
    {
        if(!mbOnlyTracking)
            s << "SLAM MODE |  ";
        else
            s << "LOCALIZATION | ";
        int nKFs = mpMap->KeyFramesInMap();
        int nMPs = mpMap->MapPointsInMap();
        s << "KFs: " << nKFs << ", MPs: " << nMPs << ", Matches: " << mnTracked;
        if(mnTrackedVO>0)
            s << ", + VO matches: " << mnTrackedVO;
    }
    else if(nState==Tracking::LOST)
    {
        s << " TRACK LOST. TRYING TO RELOCALIZE ";
    }
    else if(nState==Tracking::SYSTEM_NOT_READY)
    {
        s << " LOADING ORB VOCABULARY. PLEASE WAIT...";
    }

    int baseline=0;
    cv::Size textSize = cv::getTextSize(s.str(),cv::FONT_HERSHEY_PLAIN,1,1,&baseline);

    imText = cv::Mat(im.rows+textSize.height+10,im.cols,im.type());
    im.copyTo(imText.rowRange(0,im.rows).colRange(0,im.cols));
    imText.rowRange(im.rows,imText.rows) = cv::Mat::zeros(textSize.height+10,im.cols,im.type());
    cv::putText(imText,s.str(),cv::Point(5,imText.rows-5),cv::FONT_HERSHEY_PLAIN,1,cv::Scalar(255,255,255),1,8);

}

void FrameDrawer::Update(Tracking *pTracker)
{

    unique_lock<mutex> lock(mMutex);

    mpbuildings = pTracker->mpbuildings;
    ab_Twc = pTracker->initial_Twc.clone();
    ca_Twc = (pTracker->mCurrentFrame).mTwc.clone();
    ca_Twc_sensor = (pTracker->mCurrentFrame).mTwc_sensor.clone();
    rela_yaw = (pTracker->mCurrentFrame).mrela_yaw;
    
    pTracker->mImask.copyTo(mImask);
    pTracker->mImColor.copyTo(mIm);

    id = pTracker->mCurrentFrame.mnId;
    
    mvCurrentKeys=pTracker->mCurrentFrame.mvKeys;
    N = mvCurrentKeys.size();
    mvbVO = vector<bool>(N,false);
    mvbMap = vector<bool>(N,false);
    mbOnlyTracking = pTracker->mbOnlyTracking;


    if(pTracker->mLastProcessedState==Tracking::NOT_INITIALIZED)
    {
        mvIniKeys=pTracker->mInitialFrame.mvKeys;
        mvIniMatches=pTracker->mvIniMatches;
    }
    else if(pTracker->mLastProcessedState==Tracking::OK)
    {
        for(int i=0;i<N;i++)
        {
            MapPoint* pMP = pTracker->mCurrentFrame.mvpMapPoints[i];
            if(pMP)
            {
                if(!pTracker->mCurrentFrame.mvbOutlier[i])
                {
                    if(pMP->Observations()>0)
                        mvbMap[i]=true;
                    else
                        mvbVO[i]=true;
                }
            }
        }
    }
    mState=static_cast<int>(pTracker->mLastProcessedState);
}


} //namespace ORB_SLAM
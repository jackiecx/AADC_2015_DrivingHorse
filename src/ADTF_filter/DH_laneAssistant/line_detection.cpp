
#include "line_detection.h"

using namespace cv;

bool isPointInsideImage(Point &point, Mat &image)
{
    return point.y >=0 && point.x >= 0 && point.y < image.rows && point.x < image.cols;
}



void addNewNotVisitiedPoints(std::vector<Point> &to_visit, Mat &visited, Point starting_point)
{
    Point directions[4] = {Point(-1,0), Point(0,-1), Point(1,0), Point(0,1)};
    for(int d_idx=0; d_idx < 4; ++d_idx)
    {
        Point next_point = starting_point + directions[d_idx];
        if(isPointInsideImage(next_point, visited))
        {
            if(visited.at<uchar>(next_point)==0)
            {
                to_visit.push_back(next_point);
            }
        }
    }
}

void fillRegion(Mat &white_regions,Point &starting_point, uchar fill_value)
{
    Mat visited;
    visited.create(white_regions.rows, white_regions.cols, CV_8UC1);
    visited.setTo(0);
    std::vector<Point> to_visit;

    to_visit.push_back(starting_point);
    do
    {
        Point current_point = to_visit.back();
        to_visit.pop_back();
        visited.at<uchar>(current_point) = 255;
        if(white_regions.at<uchar>(current_point) == 255)
        {
            white_regions.at<uchar>(current_point) = fill_value;
            visited.at<uchar>(current_point) = 255;
            addNewNotVisitiedPoints(to_visit, visited, current_point);
        }
    }while(to_visit.size()>0);
}

void removeSmallRegions(Mat &white_regions, std::vector<ConnectedComponent>& connected_components, int predicted_line_width)
{
    const int threshold = predicted_line_width * predicted_line_width;

    for(std::vector<ConnectedComponent>::iterator compo_iter = connected_components.begin();
            compo_iter != connected_components.end(); )
    {
        if( compo_iter->number_of_pixels < threshold )
        {
            fillRegion(white_regions,compo_iter->starting_point, 0);
            connected_components.erase( compo_iter );
        }else
        {
            compo_iter++;
        }
    }
}

void countNumberOfLineTypes(Mat &image,Mat &line_type_image, Point &starting_point, int &number_of_thin_line_points, int &number_of_thick_line_points)
{
	Mat visited;
	visited.create(image.size(), CV_8UC1);
	visited.setTo(0);
	
    std::vector<Point> to_visit;
    to_visit.push_back(starting_point);

    uchar components_value = image.at<uchar>(starting_point);
    do
    {
        Point current_point = to_visit.back();
        to_visit.pop_back();
        if(image.at<uchar>(current_point) == components_value)
        {
            visited.at<uchar>(current_point) = 255;
            if(  line_type_image.at<uchar>(current_point) == 100 )
                number_of_thin_line_points++;
            else if(  line_type_image.at<uchar>(current_point) == 200 )
	            number_of_thick_line_points++;
	            
	        addNewNotVisitiedPoints(to_visit, visited, current_point);
        }
    }while(to_visit.size()>0);

}

void markLinesAsCentralOrRightByMajorityVote(Mat &imgTh, std::vector<ConnectedComponent>& connected_components, Mat &imgLines)
{
    for(unsigned int idx=0; idx < connected_components.size(); idx++)
    {
    	int number_of_thin_line_points=0, number_of_thick_line_points=0;
    	countNumberOfLineTypes(imgTh,imgLines, connected_components[idx].starting_point, number_of_thin_line_points, number_of_thick_line_points);
    	if (number_of_thin_line_points>number_of_thick_line_points)
	        fillRegion(imgTh,connected_components[idx].starting_point, 100);
		else
			fillRegion(imgTh,connected_components[idx].starting_point, 200);
	        
    }

}

ConnectedComponent followConnectedComponent(Mat &image, Mat &visited, Point &starting_point)
{
    ConnectedComponent new_component;
    new_component.starting_point = starting_point;
    new_component.max_expansion_point = starting_point;
    new_component.min_expansion_point = starting_point;
    new_component.number_of_pixels = 0;
    std::vector<Point> to_visit;
    to_visit.push_back(starting_point);

    uchar components_value = image.at<uchar>(starting_point);
    do
    {
        Point current_point = to_visit.back();
        to_visit.pop_back();
        if(image.at<uchar>(current_point) == components_value)
        {
            visited.at<uchar>(current_point) = 255;
            if(current_point.x < new_component.min_expansion_point.x)
                new_component.min_expansion_point.x = current_point.x;
            if(current_point.y < new_component.min_expansion_point.y)
                new_component.min_expansion_point.y = current_point.y;
            if(current_point.x > new_component.max_expansion_point.x)
                new_component.max_expansion_point.x = current_point.x;
            if(current_point.y > new_component.max_expansion_point.y)
                new_component.max_expansion_point.y = current_point.y;

            new_component.number_of_pixels++;
            addNewNotVisitiedPoints(to_visit, visited, current_point);
        }
    }while(to_visit.size()>0);
    return new_component;
}



void determineConnectedComponents(Mat &white_regions, std::vector<ConnectedComponent>& connected_components)
{
    Mat visited;
    int label = 0;

    visited.create(white_regions.rows, white_regions.cols, CV_8UC1);
    visited.setTo(0);
    for(int row = 0; row < white_regions.rows; row++)
        for(int col = 0; col < white_regions.cols; col++)
        {
            Point current_point = Point(col, row);
            if(visited.at<uchar>(current_point) == 0)
            {
                ConnectedComponent new_component = followConnectedComponent(white_regions, visited, current_point);

                if(white_regions.at<uchar>(current_point) == 255)
                {
                    new_component.label = label++;
                    connected_components.push_back(new_component);
                }
            }
        }
}



void detectWhiteRegions(Mat &input_image, Mat &output_image, int tau=30)
{
    output_image.create(input_image.size(),CV_8UC1);

    output_image.setTo(0);

    int aux=0;

    for(int row_idx=0; row_idx<output_image.rows; row_idx++)
    {
        unsigned char *pt_row_input = input_image.ptr<uchar>(row_idx);
        unsigned char *pt_row_output = output_image.ptr<uchar>(row_idx);

        for(int col_idx = tau; col_idx < output_image.cols-tau; col_idx++)
        {
            if(pt_row_input[col_idx] != 0)
            {
                aux = 2*pt_row_input[col_idx];
                aux += -pt_row_input[col_idx - tau];
                aux += -pt_row_input[col_idx + tau];
                aux += -abs((int)(pt_row_input[col_idx - tau] - pt_row_input[col_idx + tau]));

                aux = aux < 0 ? 0 : aux;
                aux = aux > 255 ? 255 : aux;

                pt_row_output[col_idx] = (unsigned char)aux > 25 ? 255 : 0;

            }
        }
    }

    medianBlur(output_image, output_image, 5);

}

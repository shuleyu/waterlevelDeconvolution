#include<iostream>
#include<fstream>
#include<sstream>
#include<vector>
#include<algorithm>
#include<cmath>
#include<unistd.h>
#include<iomanip>

#include<GMTPlotSignal.hpp>
#include<SACSignals.hpp>
#include<Float2String.hpp>

using namespace std;

// Inputs. ------------------------

double plotXMin = -40, plotXMax = 40;

const string plotRange = "-R/" + to_string(plotXMin) + "/" + to_string(plotXMax) + "/-1.2/1.2";

const vector<double> waterLevels{0.0001, 0.001, 0.01, 0.05, 0.1, 0.25, 0.5};

const vector<double> gaussianHalfHeightWidth{0.05, 0.1, 0.2, 0.35, 0.5, 1, 2, 3, 4, 5};

const vector<double> highPassFilterCorner{0.001, 0.005, 0.01, 0.02, 0.03, 0.05, 0.1, 0.15};

const string eventName = "Event_2012.12.21.22.28.08.80";
const string signalSACFile = "/Users/shuleyu/Documents/Research/t010.SamDecon/ExamplePcPevents/" + eventName + "/PcP.sac";
const string sourceSACFile = "/Users/shuleyu/Documents/Research/t010.SamDecon/ExamplePcPevents/" + eventName + "/P.sac";

// Outputs. ------------------------

const string outputSACFile = "/Users/shuleyu/Documents/Research/t010.SamDecon/ExamplePcPevents/" + eventName + "/decon.sac";

double widthToSigma(double halfHeightWidth) {

    return halfHeightWidth / 2.0 / sqrt(2 * log(2));
}


int main(){

    SACSignals signalSAC(vector<string> {signalSACFile});
    SACSignals sourceSAC(vector<string> {sourceSACFile});


    // Find Peak and normalize to Peak.
    sourceSAC.FindPeakAround(25, 10, true);
    sourceSAC.NormalizeToGlobal();
    sourceSAC.ShiftTimeReferenceToPeak();
    sourceSAC.HannTaper();

    signalSAC.FindPeakAround(25, 10, true);
    signalSAC.NormalizeToGlobal();
    signalSAC.ShiftTimeReferenceToPeak();
    signalSAC.HannTaper();


    auto sourceTrace = sourceSAC.GetData()[0];
    auto signalTrace = signalSAC.GetData()[0];


    // process and plot.

    const double YSIZE = 1 + 2.5 * gaussianHalfHeightWidth.size(), XSIZE = 1 + 6 * (1 + highPassFilterCorner.size());
    string YP = "";
    string outfile = ""; 


    // plot difference waterlevel result (different pages).

    for (auto wl: waterLevels){

        if (outfile == "") {

            outfile = GMT::BeginEasyPlot(XSIZE, YSIZE);
        }
        else {

            GMT::NewPage(outfile);
        }

        YP = "-Yf" + to_string(YSIZE - (0 + 1) * 2.5) + "i";
        GMT::MoveReferencePoint(outfile,"-Xf1i " + YP);

        GMT::psbasemap(outfile, "-JX5i/1.5i " + plotRange + " -Bxa10f2 -Bya0.5f0.1 -Bx+l\"sec.\" -By+l\"Normalized Amplitude\" -BWSne -O -K");
        GMT::psxy(outfile, signalTrace, "-JX5i/1.5i " + plotRange + " -W2p,black -O -K");
        GMT::psxy(outfile, sourceTrace, "-JX5i/1.5i " + plotRange + " -W1p,red -O -K");



        YP = "-Yf" + to_string(YSIZE - (1 + 1) * 2.5) + "i";
        GMT::MoveReferencePoint(outfile,"-Xf1i " + YP);

        vector<GMT::Text> texts;
        texts.push_back(GMT::Text(0, 0, eventName, 30, "CM"));
        texts.push_back(GMT::Text(0, -1, "water level = " + Float2String(wl, 4), 30, "CM"));
        GMT::pstext(outfile, texts, "-JX5i/1.5i " + plotRange + " -N -O -K");
//         GMT::pstext(outfile, texts, "-JX5i/1.5i " + plotRange + " -Bxa10f2 -Bya0.5f0.1 -N -O -K");



        int YCnt = 0;
        for (auto gaussianWidth: gaussianHalfHeightWidth) {


            int XCnt = 0;

            for (auto highPassCorner: highPassFilterCorner) {

                auto DeconResult = signalTrace;
                DeconResult.WaterLevelDecon(sourceTrace, wl);
                DeconResult.FindPeakAround(0, 10, true);
                DeconResult.ShiftTimeReferenceToPeak();
                DeconResult.NormalizeToSignal();

                DeconResult.GaussianBlur(widthToSigma(gaussianWidth));
                DeconResult.Butterworth(highPassCorner, 1000);
                DeconResult.FindPeakAround(0, 10, true);
                DeconResult.ShiftTimeReferenceToPeak();
                DeconResult.NormalizeToSignal();


                string XP = "-Xf" + to_string(7 + XCnt * 5 + XCnt) + "i";
                string YP = "-Yf" + to_string(YSIZE - (YCnt + 1) * 2.5) + "i";
                GMT::MoveReferencePoint(outfile, XP + " " + YP);
                GMT::psbasemap(outfile, "-JX5i/1.5i " + plotRange + " -Bxa10f2 -Bya0.5f0.1 -BWSne -O -K");
                GMT::psxy(outfile, DeconResult, "-JX5i/1.5i " + plotRange + " -W2p,purple -O -K");

                vector<GMT::Text> texts;

                if (XCnt == 0) {

                    texts.push_back(GMT::Text(plotXMin + (plotXMax - plotXMin) * 0.02, 1.0, "Gaussian = " + Float2String(gaussianWidth, 2), 15, "LT"));
                }
                if (YCnt == 0) {

                    texts.push_back(GMT::Text(plotXMin + (plotXMax - plotXMin) * 0.02, 1.4, "HighPass = " + Float2String(highPassCorner, 3) + " Hz", 15, "LB"));
                }
                
                GMT::pstext(outfile, texts, "-JX5i/1.5i " + plotRange + " -N -O -K");

                ++XCnt;
            }
            ++YCnt;
        }
    }

    GMT::SealPlot(outfile);
    GMT::ps2pdf(outfile, __FILE__);
    return 0;
}

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
#include<ShellExec.hpp>

using namespace std;

// plot parameters. ------------------------

const double plotXMin = -40, plotXMax = 40, plotYMin = -1.2, plotYMax = 1.2;

// -----------------------------------------


double widthToSigma(double halfHeightWidth) {

    return halfHeightWidth / 2.0 / sqrt(2 * log(2));
}


int main(int argc, char * argv[]){

    if (argc != 2) {

        cout << "usage: ./DeconParameters.out [parameterFile]\n";
        cout << "\n";
        cout << "The first column in [parameterFile] is an explanatory key word.\n";
        cout << "The rest columns are according paremeter(s).\n";
        cout << "The outputs is one figure [runMarker].pdf and several sac files [runMarker_wl_...].sac within the \"outputDir\" directory." << endl;
        return 1;
    }


    // read input file.

    auto waterLevels = vector<double> ();
    auto gaussianHalfHeightWidth = vector<double> ();
    auto highPassFilterCorner = vector<double> ();

    string signalSACFile = "", sourceSACFile = "", outputDir = "", runMarker = "";


    ifstream fpin;

    string inputFile(argv[1]);

    fpin.open(inputFile);

    if (!fpin.is_open()) {

        throw runtime_error("Can't open file: " + inputFile);
    }

    string line, keyWord;
    while (getline(fpin, line)) {

        if (line == "") {

            continue;
        }

        stringstream ss(line);
        double x;

        ss >> keyWord;

        if (keyWord == "runMarker") {

            ss >> runMarker;
        }
        else if (keyWord == "outputDir"){

            ss >> outputDir;
        }
        else if (keyWord == "sourceSACFile"){

            ss >> sourceSACFile;
        }
        else if (keyWord == "signalSACFile"){

            ss >> signalSACFile;
        }
        else if (keyWord == "waterLevels(%)"){

            while (ss >> x) {

                waterLevels.push_back(x);
            }
        }
        else if (keyWord == "gaussianHalfHeightWidth(sec.)"){

            while (ss >> x) {

                gaussianHalfHeightWidth.push_back(x);
            }
        }
        else if (keyWord == "highPassFilterCorner(Hz)"){

            while (ss >> x) {

                highPassFilterCorner.push_back(x);
            }
        }
        else {

            cerr << inputFile + "format error." << endl;
            return 1;
        }
    }
    fpin.close();

    if (signalSACFile == "" || sourceSACFile == "" || outputDir == "" || runMarker == "" || waterLevels.empty() || gaussianHalfHeightWidth.empty() || highPassFilterCorner.empty()){

        cerr << inputFile + "format error." << endl;
        return 1;
    }
    else {

        ShellExec("mkdir -p " + outputDir);
    }


    // WORK BEGIN.

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
    const string plotRange = "-R/" + to_string(plotXMin) + "/" + to_string(plotXMax) + "/" + to_string(plotYMin) + "/" + to_string(plotYMax);

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
        texts.push_back(GMT::Text(0, 0, runMarker, 30, "CM"));
        texts.push_back(GMT::Text(0, -1, "water level = " + Float2String(wl, 4), 30, "CM"));
        GMT::pstext(outfile, texts, "-JX5i/1.5i " + plotRange + " -N -O -K");
//         GMT::pstext(outfile, texts, "-JX5i/1.5i " + plotRange + " -Bxa10f2 -Bya0.5f0.1 -N -O -K");



        int YCnt = 0;
        for (auto gaussianWidth: gaussianHalfHeightWidth) {


            int XCnt = 0;

            for (auto highPassCorner: highPassFilterCorner) {


                // decon.
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

                // output to sac.
                auto mData = signalSAC.GetMData()[0];
                mData.stnm = "PcPDecon";

                SACSignals outSAC({DeconResult}, {mData});
                outSAC.OutputToSAC(outputDir + "/" + runMarker + "_wl_" + Float2String(wl, 4) + "_gauss_" + Float2String(gaussianWidth, 4) + "_hp_" + Float2String(highPassCorner, 4));


                // plot.
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
    GMT::ps2pdf(outfile, outputDir + "/" + runMarker, true, true);
    return 0;
}

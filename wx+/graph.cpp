// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#include "graph.h"
#include <cassert>
#include <algorithm>
#include <numeric>
#include <zen/basic_math.h>
#include <wx/settings.h>
#include "rtl.h"

using namespace zen;
using namespace numeric;


//todo: support zoom via mouse wheel

warn_static("reviewreviewreviewreviewreview")

const wxEventType zen::wxEVT_GRAPH_SELECTION = wxNewEventType();

const std::shared_ptr<LabelFormatter> Graph2D::MainAttributes::defaultFormat = std::make_shared<DecimalNumberFormatter>(); //for some buggy reason MSVC isn't able to use a temporary as a default argument

namespace
{
inline
double bestFit(double val, double low, double high) { return val < (high + low) / 2 ? low : high; }
}


double zen::nextNiceNumber(double blockSize) //round to next number which is a convenient to read block size
{
    if (blockSize <= 0)
        return 0;

    const double k = std::floor(std::log10(blockSize));
    const double e = std::pow(10, k);
    if (isNull(e))
        return 0;
    const double a = blockSize / e; //blockSize = a * 10^k with a in (1, 10)

    //have a look at leading two digits: "nice" numbers start with 1, 2, 2.5 and 5
    if (a <= 2)
        return bestFit(a, 1, 2) * e;
    else if (a <= 2.5)
        return bestFit(a, 2, 2.5) * e;
    else if (a <= 5)
        return bestFit(a, 2.5, 5) * e;
    else if (a < 10)
        return bestFit(a, 5, 10) * e;
    else
    {
        assert(false);
        return 10 * e;
    }
}


namespace
{
wxColor getDefaultColor(size_t pos)
{
    switch (pos % 10)
    {
        case 0:
            return wxColor(0, 69, 134); //blue
        case 1:
            return wxColor(255, 66, 14); //red
        case 2:
            return wxColor(255, 211, 32); //yellow
        case 3:
            return wxColor(87, 157, 28); //green
        case 4:
            return wxColor(126, 0, 33); //royal
        case 5:
            return wxColor(131, 202, 255); //light blue
        case 6:
            return wxColor(49, 64, 4); //dark green
        case 7:
            return wxColor(174, 207, 0); //light green
        case 8:
            return wxColor(75, 31, 111); //purple
        case 9:
            return wxColor(255, 149, 14); //orange
        default:
            return *wxBLACK;
    }
}


class ConvertCoord //convert between screen and input data coordinates
{
public:
    ConvertCoord(double valMin, double valMax, size_t screenSize) :
        min_(valMin),
        scaleToReal(screenSize == 0 ? 0 : (valMax - valMin) / screenSize),
        scaleToScr(isNull((valMax - valMin)) ? 0 : screenSize / (valMax - valMin)) {}

    double screenToReal(double screenPos) const //input value: [0, screenSize - 1]
    {
        return screenPos * scaleToReal + min_; //come close to valMax, but NEVER reach it!
    }
    double realToScreen(double realPos) const //return screen position in pixel (but with double precision!)
    {
        return (realPos - min_) * scaleToScr;
    }

    int realToScreenRound(double realPos) const //useful to find "proper" y-pixel positions
    {
        return numeric::round(realToScreen(realPos));
    }

private:
    double min_;
    double scaleToReal;
    double scaleToScr;
};


//enlarge range to a multiple of a "useful" block size
void widenRange(double& valMin, double& valMax, //in/out
                int& blockCount, //out
                int graphAreaSize,   //in pixel
                int optimalBlockSize, //
                const LabelFormatter& labelFmt)
{
    if (graphAreaSize > 0)
    {
        double valRangePerBlock = (valMax - valMin) * optimalBlockSize / graphAreaSize; //proposal
        valRangePerBlock = labelFmt.getOptimalBlockSize(valRangePerBlock);
        if (!isNull(valRangePerBlock))
        {
            valMin = std::floor(valMin / valRangePerBlock) * valRangePerBlock;
            valMax = std::ceil (valMax / valRangePerBlock) * valRangePerBlock;
            blockCount = numeric::round((valMax - valMin) / valRangePerBlock); //"round" to avoid IEEE 754 surprises
            return;
        }
    }
    blockCount = 0;
}


void drawXLabel(wxDC& dc, double xMin, double xMax, int blockCount, const ConvertCoord& cvrtX, const wxRect& graphArea, const wxRect& labelArea, const LabelFormatter& labelFmt)
{
    assert(graphArea.width == labelArea.width && graphArea.x == labelArea.x);
    if (blockCount <= 0)
        return;

    wxDCPenChanger dummy(dc, wxPen(wxColor(192, 192, 192))); //light grey
    wxDCTextColourChanger dummy2(dc, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)); //use user setting for labels
    dc.SetFont(wxFont(wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"Arial"));

    const double valRangePerBlock = (xMax - xMin) / blockCount;

    for (int i = 1; i < blockCount; ++i)
    {
        //draw grey vertical lines
        const double valX = xMin + i * valRangePerBlock; //step over raw data, not graph area pixels, to not lose precision
        const int x = graphArea.x + cvrtX.realToScreenRound(valX);

        if (graphArea.height > 0)
            dc.DrawLine(wxPoint(x, graphArea.y), wxPoint(x, graphArea.y + graphArea.height));

        //draw x axis labels
        const wxString label = labelFmt.formatText(xMin + i * valRangePerBlock, valRangePerBlock);
        wxSize labelExtent = dc.GetMultiLineTextExtent(label);
        dc.DrawText(label, wxPoint(x - labelExtent.GetWidth() / 2, labelArea.y + (labelArea.height - labelExtent.GetHeight()) / 2)); //center
    }
}


void drawYLabel(wxDC& dc, double yMin, double yMax, int blockCount, const ConvertCoord& cvrtY, const wxRect& graphArea, const wxRect& labelArea, const LabelFormatter& labelFmt)
{
    assert(graphArea.height == labelArea.height && graphArea.y == labelArea.y);
    if (blockCount <= 0)
        return;

    wxDCPenChanger dummy(dc, wxPen(wxColor(192, 192, 192))); //light grey
    wxDCTextColourChanger dummy2(dc, wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)); //use user setting for labels
    dc.SetFont(wxFont(wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"Arial"));

    const double valRangePerBlock = (yMax - yMin) / blockCount;

    for (int i = 1; i < blockCount; ++i)
    {
        //draw grey horizontal lines
        const double valY = yMin + i * valRangePerBlock; //step over raw data, not graph area pixels, to not lose precision
        const int y = graphArea.y + cvrtY.realToScreenRound(valY);

        if (graphArea.width > 0)
            dc.DrawLine(wxPoint(graphArea.x, y), wxPoint(graphArea.x + graphArea.width, y));

        //draw y axis labels
        const wxString label = labelFmt.formatText(valY, valRangePerBlock);
        wxSize labelExtent = dc.GetMultiLineTextExtent(label);
        dc.DrawText(label, wxPoint(labelArea.x + (labelArea.width - labelExtent.GetWidth()) / 2, y - labelExtent.GetHeight() / 2)); //center
    }
}


template <class StdContainter>
void subsample(StdContainter& cont, size_t factor)
{
    if (factor <= 1) return;

    auto iterOut = cont.begin();
    for (auto iterIn = cont.begin(); cont.end() - iterIn >= static_cast<ptrdiff_t>(factor); iterIn += factor) //don't even let iterator point out of range!
        *iterOut++ = std::accumulate(iterIn, iterIn + factor, 0.0) / static_cast<double>(factor);

    cont.erase(iterOut, cont.end());
}
}


Graph2D::Graph2D(wxWindow* parent,
                 wxWindowID winid,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style,
                 const wxString& name) : wxPanel(parent, winid, pos, size, style, name)
{
    Connect(wxEVT_PAINT, wxPaintEventHandler(Graph2D::onPaintEvent), nullptr, this);
    Connect(wxEVT_SIZE,  wxSizeEventHandler (Graph2D::onSizeEvent ),  nullptr, this);
    //http://wiki.wxwidgets.org/Flicker-Free_Drawing
    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(Graph2D::onEraseBackGround), nullptr, this);

    //SetDoubleBuffered(true); slow as hell!

#if wxCHECK_VERSION(2, 9, 1)
    SetBackgroundStyle(wxBG_STYLE_PAINT);
#else
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
#endif

    Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(Graph2D::OnMouseLeftDown), nullptr, this);
    Connect(wxEVT_MOTION,    wxMouseEventHandler(Graph2D::OnMouseMovement), nullptr, this);
    Connect(wxEVT_LEFT_UP,   wxMouseEventHandler(Graph2D::OnMouseLeftUp),   nullptr, this);
    Connect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(Graph2D::OnMouseCaptureLost), nullptr, this);
}


void Graph2D::onPaintEvent(wxPaintEvent& event)
{
    //wxAutoBufferedPaintDC dc(this); -> this one happily fucks up for RTL layout by not drawing the first column (x = 0)!
    BufferedPaintDC dc(*this, doubleBuffer);
    render(dc);
}


void Graph2D::OnMouseLeftDown(wxMouseEvent& event)
{
    activeSel.reset(new MouseSelection(*this, event.GetPosition()));

    if (!event.ControlDown())
        oldSel.clear();

    Refresh();
}


void Graph2D::OnMouseMovement(wxMouseEvent& event)
{
    if (activeSel.get())
    {
        activeSel->refCurrentPos() = event.GetPosition();
        Refresh();
    }
}


void Graph2D::OnMouseLeftUp(wxMouseEvent& event)
{
    if (activeSel.get())
    {
        if (activeSel->getStartPos() != activeSel->refCurrentPos()) //if it's just a single mouse click: discard selection
        {
            //fire off GraphSelectEvent
            GraphSelectEvent selEvent(activeSel->refSelection());
            if (wxEvtHandler* handler = GetEventHandler())
                handler->AddPendingEvent(selEvent);

            oldSel.push_back(activeSel->refSelection());
        }

        activeSel.reset();
        Refresh();
    }
}


void Graph2D::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
    activeSel.reset();
    Refresh();
}


void Graph2D::setData(const std::shared_ptr<GraphData>& data, const CurveAttributes& la)
{
    curves_.clear();
    addData(data, la);
}


void Graph2D::addData(const std::shared_ptr<GraphData>& data, const CurveAttributes& la)
{
    CurveAttributes newAttr = la;
    if (newAttr.autoColor)
        newAttr.setColor(getDefaultColor(curves_.size()));
    curves_.push_back(std::make_pair(data, newAttr));
    Refresh();
}


namespace
{
class DcBackgroundChanger
{
public:
    DcBackgroundChanger(wxDC& dc, const wxBrush& brush) : dc_(dc), old(dc.GetBackground()) { dc.SetBackground(brush); }
    ~DcBackgroundChanger() { if (old.Ok()) dc_.SetBackground(old); }
private:
    wxDC& dc_;
    const wxBrush old;
};
}


void Graph2D::render(wxDC& dc) const
{
    {
        //clear everything, set label background color
        //        const wxColor backColor = wxPanel::GetClassDefaultAttributes().colBg != wxNullColour ?
        //                                  wxPanel::GetClassDefaultAttributes().colBg :
        //                                  wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
        const wxColor backColor = GetBackgroundColour(); //user-configurable!
        DcBackgroundChanger dummy(dc, backColor); //use wxDC::SetBackground instead of wxDC::SetBrush
        dc.Clear();
    }

    //note: DON'T use wxDC::GetSize()! DC may be larger than visible area!
    /*
    -----------------------
    |        |   x-label  |
    -----------------------
    |y-label | graph area |
    |----------------------
    */
    const wxRect clientRect = GetClientSize(); //data window only
    wxRect graphArea  = clientRect; //data window only
    int xLabelPosY = clientRect.y;
    int yLabelPosX = clientRect.x;

    switch (attr.labelposX)
    {
        case X_LABEL_TOP:
            graphArea.y      += attr.xLabelHeight;
            graphArea.height -= attr.xLabelHeight;
            break;
        case X_LABEL_BOTTOM:
            xLabelPosY += clientRect.height - attr.xLabelHeight;
            graphArea.height -= attr.xLabelHeight;
            break;
        case X_LABEL_NONE:
            break;
    }

    switch (attr.labelposY)
    {
        case Y_LABEL_LEFT:
            graphArea.x     += attr.yLabelWidth;
            graphArea.width -= attr.yLabelWidth;
            break;
        case Y_LABEL_RIGHT:
            yLabelPosX += clientRect.width - attr.yLabelWidth;
            graphArea.width -= attr.yLabelWidth;
            break;
        case Y_LABEL_NONE:
            break;
    }

    {
        //paint actual graph background (without labels)
        DcBackgroundChanger dummy(dc, *wxWHITE); //accessibility: we have to set both back- and foreground colors or none at all!
        wxDCPenChanger dummy2(dc, wxColour(130, 135, 144)); //medium grey, the same Win7 uses for other frame borders
        //dc.DrawRectangle(static_cast<const wxRect&>(graphArea).Inflate(1, 1)); //correct wxWidgets design mistakes
        dc.DrawRectangle(graphArea);
        graphArea.Deflate(1, 1); //do not draw on border
    }

    //set label areas respecting graph area border!
    wxRect xLabelArea(graphArea.x, xLabelPosY, graphArea.width, attr.xLabelHeight);
    wxRect yLabelArea(yLabelPosX, graphArea.y, attr.yLabelWidth, graphArea.height);

    const wxPoint graphAreaOrigin = graphArea.GetTopLeft();

    //detect x value range
    double minX = attr.minXauto ?  std::numeric_limits<double>::infinity() : attr.minX; //automatic: ensure values are initialized by first curve
    double maxX = attr.maxXauto ? -std::numeric_limits<double>::infinity() : attr.maxX; //
    if (!curves_.empty())
        for (auto it = curves_.begin(); it != curves_.end(); ++it)
            if (it->first.get())
            {
                const GraphData& graph = *it->first;
                assert(graph.getXBegin() <= graph.getXEnd() + 1.0e-9);
                //GCC fucks up badly when comparing two *binary identical* doubles and finds "begin > end" with diff of 1e-18

                if (attr.minXauto)
                    minX = std::min(minX, graph.getXBegin());
                if (attr.maxXauto)
                    maxX = std::max(maxX, graph.getXEnd());
            }

    if (minX < maxX && maxX - minX < std::numeric_limits<double>::infinity()) //valid x-range
    {
        int blockCountX = 0;
        //enlarge minX, maxX to a multiple of a "useful" block size
        if (attr.labelposX != X_LABEL_NONE && attr.labelFmtX.get())
            widenRange(minX, maxX, //in/out
                       blockCountX, //out
                       graphArea.width,
                       dc.GetTextExtent(L"100000000000000").GetWidth(),
                       *attr.labelFmtX);

        //detect y value range
        std::vector<std::pair<std::vector<double>, int>> yValuesList(curves_.size());
        double minY = attr.minYauto ?  std::numeric_limits<double>::infinity() : attr.minY; //automatic: ensure values are initialized by first curve
        double maxY = attr.maxYauto ? -std::numeric_limits<double>::infinity() : attr.maxY; //
        {
            const int AVG_FACTOR = 2; //some averaging of edgy input data to smoothen behavior on window resize
            const ConvertCoord cvrtX(minX, maxX, graphArea.width * AVG_FACTOR);

            for (auto it = curves_.begin(); it != curves_.end(); ++it)
                if (it->first.get())
                {
                    const size_t index = it - curves_.begin();
                    const GraphData& graph = *it->first;

                    std::vector<double>& yValues = yValuesList[index].first;  //actual y-values
                    int& offsetX                 = yValuesList[index].second; //x-value offset in pixel
                    {
                        const double xBegin = graph.getXBegin();
                        const double xEnd   = graph.getXEnd();

                        const int posFirst = std::ceil(cvrtX.realToScreen(std::max(xBegin, minX))); //apply min/max *before* calling realToScreen()!
                        const int posLast  = std::ceil(cvrtX.realToScreen(std::min(xEnd,   maxX))); //do not step outside [xBegin, xEnd) range => 2 x ceil!
                        //conversion from std::ceil double to int is loss-free for full value range of int! tested successfully on MSVC

                        for (int i = posFirst; i < posLast; ++i)
                            yValues.push_back(graph.getValue(cvrtX.screenToReal(i)));

                        subsample(yValues, AVG_FACTOR);
                        offsetX = posFirst / AVG_FACTOR;
                    }

                    if (!yValues.empty())
                    {
                        if (attr.minYauto)
                            minY = std::min(minY, *std::min_element(yValues.begin(), yValues.end()));
                        if (attr.maxYauto)
                            maxY = std::max(maxY, *std::max_element(yValues.begin(), yValues.end()));
                    }
                }
        }
        if (minY < maxY) //valid y-range
        {
            int blockCountY = 0;
            //enlarge minY, maxY to a multiple of a "useful" block size
            if (attr.labelposY != Y_LABEL_NONE && attr.labelFmtY.get())
                widenRange(minY, maxY, //in/out
                           blockCountY, //out
                           graphArea.height,
                           3 * dc.GetTextExtent(L"1").GetHeight(),
                           *attr.labelFmtY);

            const ConvertCoord cvrtX(minX, maxX, graphArea.width); //map [minX, maxX) to [0, graphWidth)
            const ConvertCoord cvrtY(maxY, minY, graphArea.height <= 0 ? 0 : graphArea.height - 1); //map [minY, maxY] to [graphHeight - 1, 0]

            //calculate curve coordinates on graph area
            auto getCurvePoints = [&](size_t index, std::vector<wxPoint>& points)
            {
                if (index < yValuesList.size())
                {
                    const std::vector<double>& yValues = yValuesList[index].first;  //actual y-values
                    const int offsetX                  = yValuesList[index].second; //x-value offset in pixel

                    for (auto i = yValues.begin(); i != yValues.end(); ++i)
                        points.push_back(wxPoint(offsetX + (i - yValues.begin()),
                                                 cvrtY.realToScreenRound(*i)) + graphAreaOrigin);
                }
            };

            //update active mouse selection
            if (activeSel.get() &&
                graphArea.width  > 0  && graphArea.height > 0)
            {
                wxPoint startPos   = activeSel->getStartPos()   - graphAreaOrigin; //pos relative to graphArea
                wxPoint currentPos = activeSel->refCurrentPos() - graphAreaOrigin;

                //normalize positions: a mouse selection is symmetric and *not* an half-open range!
                confine(startPos  .x, 0, graphArea.width - 1);
                confine(currentPos.x, 0, graphArea.width - 1);
                confine(startPos  .y, 0, graphArea.height - 1);
                confine(currentPos.y, 0, graphArea.height - 1);

                auto& from = activeSel->refSelection().from;
                auto& to   = activeSel->refSelection().to;

                //save current selection as double coordinates
                from.x = cvrtX.screenToReal(startPos  .x + (startPos.x <= currentPos.x ? 0 : 1)); // use full pixel range for selection!
                to  .x = cvrtX.screenToReal(currentPos.x + (startPos.x <= currentPos.x ? 1 : 0));

                from.y = cvrtY.screenToReal(startPos  .y + (startPos.y <= currentPos.y ? 0 : 1));
                to  .y = cvrtY.screenToReal(currentPos.y + (startPos.y <= currentPos.y ? 1 : 0));
            }

            //#################### begin drawing ####################
            //1. draw colored area under curves
            for (auto it = curves_.begin(); it != curves_.end(); ++it)
                if (it->second.drawCurveArea)
                {
                    std::vector<wxPoint> points;
                    getCurvePoints(it - curves_.begin(), points);
                    if (!points.empty())
                    {
                        points.push_back(wxPoint(points.back ().x, graphArea.GetBottom())); //add lower right and left corners
                        points.push_back(wxPoint(points.front().x, graphArea.GetBottom())); //

                        wxDCBrushChanger dummy(dc, it->second.fillColor);
                        wxDCPenChanger  dummy2(dc, it->second.fillColor);
                        dc.DrawPolygon(static_cast<int>(points.size()), &points[0]);
                    }
                }

            //2. draw all currently set mouse selections (including active selection)
            std::vector<SelectionBlock> allSelections = oldSel;
            if (activeSel)
                allSelections.push_back(activeSel->refSelection());
            {
                //alpha channel (not yet) supported on wxMSW, so draw selection before curves
                wxDCBrushChanger dummy(dc, wxColor(168, 202, 236)); //light blue
                wxDCPenChanger  dummy2(dc, wxColor(51,  153, 255)); //dark blue

                for (auto i = allSelections.begin(); i != allSelections.end(); ++i)
                {
                    //harmonize with active mouse selection above!
                    wxPoint pixelFrom(cvrtX.realToScreenRound(i->from.x),
                                      cvrtY.realToScreenRound(i->from.y));
                    wxPoint pixelTo(cvrtX.realToScreenRound(i->to.x),
                                    cvrtY.realToScreenRound(i->to.y));
					 //convert half-open to inclusive ranges for use with wxDC::DrawRectangle
                    if (pixelFrom.x != pixelTo.x) //no matter how small the selection, always draw at least one pixel!
                    {
                        pixelFrom.x -= pixelFrom.x < pixelTo.x ? 0 : 1;
                        pixelTo  .x -= pixelFrom.x < pixelTo.x ? 1 : 0;
                    }
                    if (pixelFrom.y != pixelTo.y)
                    {
                        pixelFrom.y -= pixelFrom.y < pixelTo.y ? 0 : 1;
                        pixelTo  .y -= pixelFrom.y < pixelTo.y ? 1 : 0;
                    }
                    confine(pixelFrom.x, 0, graphArea.width - 1);
                    confine(pixelTo  .x, 0, graphArea.width - 1);
                    confine(pixelFrom.y, 0, graphArea.height - 1);
                    confine(pixelTo  .y, 0, graphArea.height - 1);

                    pixelFrom += graphAreaOrigin;
                    pixelTo   += graphAreaOrigin;

                    switch (attr.mouseSelMode)
                    {
                        case SELECT_NONE:
                            break;
                        case SELECT_RECTANGLE:
                            dc.DrawRectangle(wxRect(pixelFrom, pixelTo));
                            break;
                        case SELECT_X_AXIS:
                            dc.DrawRectangle(wxRect(wxPoint(pixelFrom.x, graphArea.y), wxPoint(pixelTo.x, graphArea.y + graphArea.height - 1)));
                            break;
                        case SELECT_Y_AXIS:
                            dc.DrawRectangle(wxRect(wxPoint(graphArea.x, pixelFrom.y), wxPoint(graphArea.x + graphArea.width - 1, pixelTo.y)));
                            break;
                    }
                }
            }

            //3. draw labels and background grid
            drawXLabel(dc, minX, maxX, blockCountX, cvrtX, graphArea, xLabelArea, *attr.labelFmtX);
            drawYLabel(dc, minY, maxY, blockCountY, cvrtY, graphArea, yLabelArea, *attr.labelFmtY);

            //4. finally draw curves
            for (auto it = curves_.begin(); it != curves_.end(); ++it)
            {
                std::vector<wxPoint> points;
                getCurvePoints(it - curves_.begin(), points);
                if (!points.empty())
                {
                    wxDCPenChanger dummy(dc, wxPen(it->second.color, it->second.lineWidth));
                    dc.DrawLines(static_cast<int>(points.size()), &points[0]);
                    dc.DrawPoint(points.back()); //last pixel omitted by DrawLines
                }
            }
        }
    }
}

// **************************************************************************
// * This file is part of the zenXML project. It is distributed under the   *
// * Boost Software License, Version 1.0. See accompanying file             *
// * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt.       *
// * Copyright (C) 2011 ZenJu (zhnmju123 AT gmx.de)                         *
// **************************************************************************

#include "graph.h"
#include <cassert>
#include <algorithm>
#include <numeric>
#include <zen/basic_math.h>
#include <wx/settings.h>

using namespace zen;
using namespace numeric;


//todo: support zoom via mouse wheel


const wxEventType zen::wxEVT_GRAPH_SELECTION = wxNewEventType();

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
    pos %= 10;
    switch (pos)
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


void drawYLabel(wxDC& dc, double& yMin, double& yMax, const wxRect& clientArea, int labelWidth, bool drawLeft, const LabelFormatter& labelFmt) //clientArea := y-label + data window
{
    //note: DON'T use wxDC::GetSize()! DC may be larger than visible area!
    if (clientArea.GetHeight() <= 0 || clientArea.GetWidth() <= 0) return;

    int optimalBlockHeight = 3 * dc.GetMultiLineTextExtent(wxT("1")).GetHeight();;

    double valRangePerBlock = (yMax - yMin) * optimalBlockHeight / clientArea.GetHeight();
    valRangePerBlock = labelFmt.getOptimalBlockSize(valRangePerBlock);
    if (numeric::isNull(valRangePerBlock)) return;

    double yMinNew = std::floor(yMin / valRangePerBlock) * valRangePerBlock;
    double yMaxNew = std::ceil (yMax / valRangePerBlock) * valRangePerBlock;
    int blockCount = numeric::round((yMaxNew - yMinNew) / valRangePerBlock);
    if (blockCount == 0) return;

    yMin = yMinNew; //inform about adjusted y value range
    yMax = yMaxNew;

    //draw labels
    {
        wxDCPenChanger dummy(dc, wxPen(wxColor(192, 192, 192))); //light grey
        dc.SetFont(wxFont(wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ));

        const int posLabel      = drawLeft ? 0 : clientArea.GetWidth() - labelWidth;
        const int posDataArea   = drawLeft ? labelWidth : 0;
        const int widthDataArea = clientArea.GetWidth() - labelWidth;

        const wxPoint origin = clientArea.GetTopLeft();

        for (int i = 1; i < blockCount; ++i)
        {
            //draw grey horizontal lines
            const int y = i * static_cast<double>(clientArea.GetHeight()) / blockCount;
            if (widthDataArea > 0)
                dc.DrawLine(wxPoint(posDataArea, y) + origin, wxPoint(posDataArea + widthDataArea - 1, y) + origin);

            //draw y axis labels
            const wxString label = labelFmt.formatText(yMaxNew - i * valRangePerBlock ,valRangePerBlock);
            wxSize labelExtent = dc.GetMultiLineTextExtent(label);

            labelExtent.x = std::max(labelExtent.x, labelWidth); //enlarge if possible to center horizontally

            dc.DrawLabel(label, wxRect(wxPoint(posLabel, y - labelExtent.GetHeight() / 2) + origin, labelExtent), wxALIGN_CENTRE);
        }
    }
}


void drawXLabel(wxDC& dc, double& xMin, double& xMax, const wxRect& clientArea, int labelHeight, bool drawBottom, const LabelFormatter& labelFmt) //clientArea := x-label + data window
{
    //note: DON'T use wxDC::GetSize()! DC may be larger than visible area!
    if (clientArea.GetHeight() <= 0 || clientArea.GetWidth() <= 0) return;

    int optimalBlockWidth = dc.GetMultiLineTextExtent(wxT("100000000000000")).GetWidth();

    double valRangePerBlock = (xMax - xMin) * optimalBlockWidth / clientArea.GetWidth();
    valRangePerBlock = labelFmt.getOptimalBlockSize(valRangePerBlock);
    if (numeric::isNull(valRangePerBlock)) return;

    double xMinNew = std::floor(xMin / valRangePerBlock) * valRangePerBlock;
    double xMaxNew = std::ceil (xMax / valRangePerBlock) * valRangePerBlock;
    int blockCount = numeric::round((xMaxNew - xMinNew) / valRangePerBlock);
    if (blockCount == 0) return;

    xMin = xMinNew; //inform about adjusted x value range
    xMax = xMaxNew;

    //draw labels
    {
        wxDCPenChanger dummy(dc, wxPen(wxColor(192, 192, 192))); //light grey
        dc.SetFont(wxFont(wxNORMAL_FONT->GetPointSize(), wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial") ));

        const int posLabel       = drawBottom ? clientArea.GetHeight() - labelHeight : 0;
        const int posDataArea    = drawBottom ? 0 : labelHeight;
        const int heightDataArea = clientArea.GetHeight() - labelHeight;

        const wxPoint origin = clientArea.GetTopLeft();

        for (int i = 1; i < blockCount; ++i)
        {
            //draw grey vertical lines
            const int x = i * static_cast<double>(clientArea.GetWidth()) / blockCount;
            if (heightDataArea > 0)
                dc.DrawLine(wxPoint(x, posDataArea) + origin, wxPoint(x, posDataArea + heightDataArea - 1) + origin);

            //draw x axis labels
            const wxString label = labelFmt.formatText(xMin + i * valRangePerBlock ,valRangePerBlock);
            wxSize labelExtent = dc.GetMultiLineTextExtent(label);

            labelExtent.y = std::max(labelExtent.y, labelHeight); //enlarge if possible to center vertically

            dc.DrawLabel(label, wxRect(wxPoint(x - labelExtent.GetWidth() / 2, posLabel) + origin, labelExtent), wxALIGN_CENTRE);
        }
    }
}


class ConvertCoord //convert between screen and actual coordinates
{
public:
    ConvertCoord(double valMin, double valMax, size_t screenSize) :
        min_(valMin),
        scaleToReal(screenSize == 0 ? 0 :                (valMax - valMin) / screenSize),
        scaleToScr(numeric::isNull(valMax - valMin) ? 0 : screenSize / (valMax - valMin)) {}

    double screenToReal(double screenPos) const //input value: [0, screenSize - 1]
    {
        return screenPos * scaleToReal + min_; //come close to valMax, but NEVER reach it!
    }
    double realToScreen(double realPos) const //return screen position in pixel (but with double precision!)
    {
        return (realPos - min_) * scaleToScr;
    }

private:
    const double min_;
    const double scaleToReal;
    const double scaleToScr;
};


template <class StdCont>
void subsample(StdCont& cont, size_t factor)
{
    if (factor <= 1) return;

    typedef typename StdCont::iterator IterType;

    IterType posWrite = cont.begin();
    for (IterType posRead = cont.begin(); cont.end() - posRead >= static_cast<int>(factor); posRead += factor) //don't even let iterator point out of range!
        *posWrite++ = std::accumulate(posRead, posRead + factor, 0.0) / static_cast<double>(factor);

    cont.erase(posWrite, cont.end());
}
}


Graph2D::Graph2D(wxWindow* parent,
                 wxWindowID winid,
                 const wxPoint& pos,
                 const wxSize& size,
                 long style,
                 const wxString& name) :
    wxPanel(parent, winid, pos, size, style, name)
{
    Connect(wxEVT_PAINT, wxPaintEventHandler(Graph2D::onPaintEvent), NULL, this);
    Connect(wxEVT_SIZE, wxEventHandler(Graph2D::onRefreshRequired),  NULL, this);
    //http://wiki.wxwidgets.org/Flicker-Free_Drawing
    Connect(wxEVT_ERASE_BACKGROUND, wxEraseEventHandler(Graph2D::onEraseBackGround), NULL, this);

#if wxCHECK_VERSION(2, 9, 1)
    SetBackgroundStyle(wxBG_STYLE_PAINT);
#else
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
#endif

    Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(Graph2D::OnMouseLeftDown), NULL, this);
    Connect(wxEVT_MOTION,    wxMouseEventHandler(Graph2D::OnMouseMovement), NULL, this);
    Connect(wxEVT_LEFT_UP,   wxMouseEventHandler(Graph2D::OnMouseLeftUp),   NULL, this);
    Connect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(Graph2D::OnMouseCaptureLost), NULL, this);
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
            GraphSelectEvent evt(activeSel->refSelection());
            GetEventHandler()->AddPendingEvent(evt);

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


void Graph2D::setData(const std::shared_ptr<GraphData>& data, const LineAttributes& la)
{
    curves_.clear();
    addData(data, la);
}


void Graph2D::addData(const std::shared_ptr<GraphData>& data, const LineAttributes& la)
{
    LineAttributes newAttr = la;
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
        //have everything including label background in natural window color by default (overwriting current background color)
        DcBackgroundChanger dummy(dc, wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)); //sigh, who *invents* this stuff??? -> workaround for issue with wxBufferedPaintDC
        //wxDCBrushChanger dummy(dc, *wxTRANSPARENT_BRUSH);
        dc.Clear();
    }

    //note: DON'T use wxDC::GetSize()! DC may be larger than visible area!
    /*
    -----------------------
    |y-label |data window |
    |----------------------
    |        |   x-label  |
    -----------------------
    */
    wxRect dataArea   = GetClientSize(); //data window only
    wxRect yLabelArea = GetClientSize(); //y-label + data window
    wxRect xLabelArea = GetClientSize(); //x-label + data window

    switch (attr.labelposX)
    {
        case X_LABEL_TOP:
            dataArea.y      += attr.labelHeightX;
            dataArea.height -= attr.labelHeightX;
            yLabelArea = dataArea;
            break;
        case X_LABEL_BOTTOM:
            dataArea.height -= attr.labelHeightX;
            yLabelArea = dataArea;
            break;
        case X_LABEL_NONE:
            break;
    }

    switch (attr.labelposY)
    {
        case Y_LABEL_LEFT:
            dataArea  .x += attr.labelWidthY;
            xLabelArea.x += attr.labelWidthY;
            dataArea  .width -= attr.labelWidthY;
            xLabelArea.width -= attr.labelWidthY;
            break;
        case Y_LABEL_RIGHT:
            dataArea  .width -= attr.labelWidthY;
            xLabelArea.width -= attr.labelWidthY;
            break;
        case Y_LABEL_NONE:
            break;
    }

    {
        //paint actual graph background (without labels) using window background color
        DcBackgroundChanger dummy(dc, GetBackgroundColour());
        wxDCPenChanger dummy2(dc, wxColour(130, 135, 144)); //medium grey, the same Win7 uses for other frame borders
        //dc.DrawRectangle(static_cast<const wxRect&>(dataArea).Inflate(1, 1)); //correct wxWidgets design mistakes
        dc.DrawRectangle(dataArea);
        dataArea.Deflate(1, 1); //do not draw on border
    }

    //detect x value range
    double minWndX = attr.minXauto ?  HUGE_VAL : attr.minX; //automatic: ensure values are initialized by first curve
    double maxWndX = attr.maxXauto ? -HUGE_VAL : attr.maxX; //
    if (!curves_.empty())
    {
        for (GraphList::const_iterator j = curves_.begin(); j != curves_.end(); ++j)
        {
            if (!j->first.get()) continue;
            const GraphData& graph = *j->first;
            assert(graph.getXBegin() <= graph.getXEnd());

            if (attr.minXauto)
                minWndX = std::min(minWndX, graph.getXBegin());
            if (attr.maxXauto)
                maxWndX = std::max(maxWndX, graph.getXEnd());
        }
        if (attr.labelposX != X_LABEL_NONE && //minWndX, maxWndX are just a suggestion, drawXLabel may enlarge them!
            attr.labelFmtX.get())
            drawXLabel(dc, minWndX, maxWndX, xLabelArea, attr.labelHeightX, attr.labelposX == X_LABEL_BOTTOM, *attr.labelFmtX);
    }
    if (minWndX < maxWndX) //valid x-range
    {
        //detect y value range
        std::vector<std::pair<std::vector<double>, int>> yValuesList(curves_.size());
        double minWndY = attr.minYauto ?  HUGE_VAL : attr.minY; //automatic: ensure values are initialized by first curve
        double maxWndY = attr.maxYauto ? -HUGE_VAL : attr.maxY; //
        if (!curves_.empty())
        {
            const int avgFactor = 2; //some averaging of edgy input data to smoothen behavior on window resize
            const ConvertCoord cvrtX(minWndX, maxWndX, dataArea.width * avgFactor);

            for (GraphList::const_iterator j = curves_.begin(); j != curves_.end(); ++j)
            {
                if (!j->first.get()) continue;
                const GraphData& graph = *j->first;

                std::vector<double>& yValues = yValuesList[j - curves_.begin()].first;  //actual y-values
                int& offset                  = yValuesList[j - curves_.begin()].second; //x-value offset in pixel
                {
                    const int posFirst = std::max<int>(std::ceil(cvrtX.realToScreen(graph.getXBegin())), 0); //evaluate visible area only and make sure to not step one pixel before xbegin()!
                    const int postLast = std::min<int>(std::floor(cvrtX.realToScreen(graph.getXEnd())), dataArea.width * avgFactor); //

                    for (int i = posFirst; i < postLast; ++i)
                        yValues.push_back(graph.getValue(cvrtX.screenToReal(i)));

                    subsample(yValues, avgFactor);
                    offset = posFirst / avgFactor;
                }

                if (!yValues.empty())
                {
                    if (attr.minYauto)
                        minWndY = std::min(minWndY, *std::min_element(yValues.begin(), yValues.end()));
                    if (attr.maxYauto)
                        maxWndY = std::max(maxWndY, *std::max_element(yValues.begin(), yValues.end()));
                }
            }
        }
        if (minWndY < maxWndY) //valid y-range
        {
            if (attr.labelposY != Y_LABEL_NONE && //minWnd, maxWndY are just a suggestion, drawYLabel may enlarge them!
                attr.labelFmtY.get())
                drawYLabel(dc, minWndY, maxWndY, yLabelArea, attr.labelWidthY, attr.labelposY == Y_LABEL_LEFT, *attr.labelFmtY);

            const ConvertCoord cvrtY(minWndY, maxWndY, dataArea.height <= 0 ? 0 : dataArea.height - 1); //both minY/maxY values will be actually evaluated in contrast to maxX => - 1
            const ConvertCoord cvrtX(minWndX, maxWndX, dataArea.width);

            const wxPoint dataOrigin = dataArea.GetTopLeft();

            //update active mouse selection
            if (activeSel.get() &&
                dataArea.width  > 0  &&
                dataArea.height > 0)
            {
                wxPoint startPos   = activeSel->getStartPos()   - dataOrigin; //pos relative to dataArea
                wxPoint currentPos = activeSel->refCurrentPos() - dataOrigin;

                //normalize positions
                confine(startPos  .x, 0, dataArea.width); //allow for one past the end(!) to enable "full range selections"
                confine(currentPos.x, 0, dataArea.width); //

                confine(startPos  .y, 0, dataArea.height); //
                confine(currentPos.y, 0, dataArea.height); //

                //save current selection as double coordinates
                activeSel->refSelection().from = SelectionBlock::Point(cvrtX.screenToReal(startPos.x + 0.5), //+0.5 start selection in the middle of a pixel
                                                                       cvrtY.screenToReal(startPos.y + 0.5));
                activeSel->refSelection().to   = SelectionBlock::Point(cvrtX.screenToReal(currentPos.x + 0.5),
                                                                       cvrtY.screenToReal(currentPos.y + 0.5));
            }
            //draw all currently set mouse selections (including active selection)
            std::vector<SelectionBlock> allSelections = oldSel;
            if (activeSel)
                allSelections.push_back(activeSel->refSelection());
            {
                wxColor colSelect(168, 202, 236); //light blue
                //wxDCBrushChanger dummy(dc, *wxTRANSPARENT_BRUSH);
                wxDCBrushChanger dummy(dc, colSelect); //alpha channel (not yet) supported on wxMSW, so draw selection before graphs

                wxPen selPen(colSelect);
                //wxPen selPen(*wxBLACK);
                //selPen.SetStyle(wxSHORT_DASH);
                wxDCPenChanger dummy2(dc, selPen);

                for (auto i = allSelections.begin(); i != allSelections.end(); ++i)
                {
                    const wxPoint pixelFrom = wxPoint(cvrtX.realToScreen(i->from.x),
                                                      cvrtY.realToScreen(i->from.y)) + dataOrigin;
                    const wxPoint pixelTo = wxPoint(cvrtX.realToScreen(i->to.x),
                                                    cvrtY.realToScreen(i->to.y)) + dataOrigin;

                    switch (attr.mouseSelMode)
                    {
                        case SELECT_NONE:
                            break;
                        case SELECT_RECTANGLE:
                            dc.DrawRectangle(wxRect(pixelFrom, pixelTo));
                            break;
                        case SELECT_X_AXIS:
                            dc.DrawRectangle(wxRect(wxPoint(pixelFrom.x, dataArea.y), wxPoint(pixelTo.x, dataArea.y + dataArea.height - 1)));
                            break;
                        case SELECT_Y_AXIS:
                            dc.DrawRectangle(wxRect(wxPoint(dataArea.x, pixelFrom.y), wxPoint(dataArea.x + dataArea.width - 1, pixelTo.y)));
                            break;
                    }
                }
            }

            //finally draw curves
            for (GraphList::const_iterator j = curves_.begin(); j != curves_.end(); ++j)
            {
                std::vector<double>& yValues = yValuesList[j - curves_.begin()].first; //actual y-values
                int offset                   = yValuesList[j - curves_.begin()].second; //x-value offset in pixel

                std::vector<wxPoint> curve;
                for (std::vector<double>::const_iterator i = yValues.begin(); i != yValues.end(); ++i)
                    curve.push_back(wxPoint(i - yValues.begin() + offset,
                                            dataArea.height - 1 - cvrtY.realToScreen(*i)) + dataOrigin); //screen y axis starts upper left

                if (!curve.empty())
                {
                    dc.SetPen(wxPen(j->second.color, j->second.lineWidth));
                    dc.DrawLines(curve.size(), &curve[0]);
                }
            }
        }
    }
}

// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) Zenju (zenju AT gmx DOT de) - All Rights Reserved        *
// **************************************************************************

#ifndef WX_PLOT_HEADER_2344252459
#define WX_PLOT_HEADER_2344252459

#include <vector>
#include <memory>
#include <wx/panel.h>
#include <wx/dcbuffer.h>
#include <zen/string_tools.h>

//simple 2D graph as wxPanel specialization

namespace zen
{
/*
Example:
    //init graph (optional)
    m_panelGraph->setAttributes(Graph2D::MainAttributes().
                                setLabelX(Graph2D::POSLX_BOTTOM, 20, std::make_shared<LabelFormatterTimeElapsed>()).
                                setLabelY(Graph2D::POSLY_RIGHT,  60, std::make_shared<LabelFormatterBytes>()));
    //set graph data
    std::shared_ptr<GraphData> graphDataBytes = ...
	m_panelGraph->setData(graphDataBytes, Graph2D::CurveAttributes().setLineWidth(2).setColor(wxColor(0, 192, 0)));
*/

//------------------------------------------------------------------------------------------------------------
struct GraphData
{
    virtual ~GraphData() {}
    virtual double getValue (double x) const = 0;
    virtual double getXBegin()         const = 0;
    virtual double getXEnd  ()         const = 0; //upper bound for x, getValue() is NOT evaluated at this position! Similar to std::vector::end()
};


//reference data implementation
class RangeData : public GraphData
{
public:
    std::vector<double>& refData() { return data; }

private:
    virtual double getValue(double x) const
    {
        const size_t pos = static_cast<size_t>(x);
        return pos < data.size() ? data[pos] : 0;
    }
    virtual double getXBegin() const { return 0; }
    virtual double getXEnd() const { return data.size(); } //example: two-element range is accessible within [0, 2)

    std::vector<double> data;
};

/*
//reference data implementation
class VectorData : public GraphData
{
public:
    operator std::vector<double>& () { return data; }

private:
    virtual double getValue(double x) const
    {
        const size_t pos = static_cast<size_t>(x);
        return pos < data.size() ? data[pos] : 0;
    }
    virtual double getXBegin() const { return 0; }
    virtual double getXEnd() const { return data.size(); } //example: two-element range is accessible within [0, 2)

    std::vector<double> data;
};
*/

//------------------------------------------------------------------------------------------------------------
struct LabelFormatter
{
    virtual ~LabelFormatter() {}

    //determine convenient graph label block size in unit of data: usually some small deviation on "sizeProposed"
    virtual double getOptimalBlockSize(double sizeProposed) const = 0;

    //create human-readable text for x or y-axis position
    virtual wxString formatText(double value, double optimalBlockSize) const = 0;
};

double nextNiceNumber(double blockSize); //round to next number which is convenient to read, e.g. 2.13 -> 2; 2.7 -> 2.5

struct DecimalNumberFormatter : public LabelFormatter
{
    virtual double getOptimalBlockSize(double sizeProposed) const { return nextNiceNumber(sizeProposed); }
    virtual wxString formatText(double value, double optimalBlockSize) const { return zen::numberTo<wxString>(value); }
};

//------------------------------------------------------------------------------------------------------------

//emit data selection event
//Usage: wnd.Connect(wxEVT_GRAPH_SELECTION, GraphSelectEventHandler(MyDlg::OnGraphSelection), nullptr, this);
//       void MyDlg::OnGraphSelection(GraphSelectEvent& event);

extern const wxEventType wxEVT_GRAPH_SELECTION;

struct SelectionBlock
{
    struct Point
    {
        Point() : x(0), y(0) {}
        Point(double xVal, double yVal) : x(xVal), y(yVal) {}
        double x;
        double y;
    };

    Point from;
    Point to;
};

class GraphSelectEvent : public wxCommandEvent
{
public:
    GraphSelectEvent(const SelectionBlock& selBlock) : wxCommandEvent(wxEVT_GRAPH_SELECTION), selBlock_(selBlock) {}
    virtual wxEvent* Clone() const { return new GraphSelectEvent(selBlock_); }

    SelectionBlock getSelection() { return selBlock_; }

private:
    SelectionBlock selBlock_;
};

typedef void (wxEvtHandler::*GraphSelectEventFunction)(GraphSelectEvent&);

#define GraphSelectEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(GraphSelectEventFunction, &func)


//------------------------------------------------------------------------------------------------------------
class Graph2D : public wxPanel
{
public:
    Graph2D(wxWindow* parent,
            wxWindowID winid     = wxID_ANY,
            const wxPoint& pos   = wxDefaultPosition,
            const wxSize& size   = wxDefaultSize,
            long style           = wxTAB_TRAVERSAL | wxNO_BORDER,
            const wxString& name = wxPanelNameStr);

    class CurveAttributes
    {
    public:
        CurveAttributes() : autoColor(true), drawCurveArea(false), lineWidth(2) {}

        CurveAttributes& setColor     (const wxColour& col) { color = col; autoColor = false; return *this; }
        CurveAttributes& fillCurveArea(const wxColour& col) { fillColor = col; drawCurveArea = true; return *this; }
        CurveAttributes& setLineWidth(size_t width) { lineWidth = static_cast<int>(width); return *this; }

    private:
        friend class Graph2D;

        bool autoColor;
        wxColour color;

        bool drawCurveArea;
        wxColour fillColor;

        int lineWidth;
    };

    void setData(const std::shared_ptr<GraphData>& data, const CurveAttributes& ca = CurveAttributes());
    void addData(const std::shared_ptr<GraphData>& data, const CurveAttributes& ca = CurveAttributes());

    enum PosLabelY
    {
        Y_LABEL_LEFT,
        Y_LABEL_RIGHT,
        Y_LABEL_NONE
    };

    enum PosLabelX
    {
        X_LABEL_TOP,
        X_LABEL_BOTTOM,
        X_LABEL_NONE
    };

    enum SelMode
    {
        SELECT_NONE,
        SELECT_RECTANGLE,
        SELECT_X_AXIS,
        SELECT_Y_AXIS,
    };

    class MainAttributes
    {
    public:
        MainAttributes() :
            minXauto(true),
            maxXauto(true),
            minX(0),
            maxX(0),
            minYauto(true),
            maxYauto(true),
            minY(0),
            maxY(0),
            labelposX(X_LABEL_BOTTOM),
            xLabelHeight(25),
            labelFmtX(std::make_shared<DecimalNumberFormatter>()),
            labelposY(Y_LABEL_LEFT),
            yLabelWidth(60),
            labelFmtY(std::make_shared<DecimalNumberFormatter>()),
            mouseSelMode(SELECT_RECTANGLE) {}

        MainAttributes& setMinX(double newMinX) { minX = newMinX; minXauto = false; return *this; }
        MainAttributes& setMaxX(double newMaxX) { maxX = newMaxX; maxXauto = false; return *this; }

        MainAttributes& setMinY(double newMinY) { minY = newMinY; minYauto = false; return *this; }
        MainAttributes& setMaxY(double newMaxY) { maxY = newMaxY; maxYauto = false; return *this; }

        MainAttributes& setAutoSize() { minXauto = maxXauto = minYauto = maxYauto = true; return *this; }

        static const std::shared_ptr<LabelFormatter> defaultFormat;

        MainAttributes& setLabelX(PosLabelX posX, size_t height = 25, const std::shared_ptr<LabelFormatter>& newLabelFmt = defaultFormat)
        {
            labelposX    = posX;
            xLabelHeight = static_cast<int>(height);
            labelFmtX    = newLabelFmt;
            return *this;
        }
        MainAttributes& setLabelY(PosLabelY posY, size_t width = 60, const std::shared_ptr<LabelFormatter>& newLabelFmt = defaultFormat)
        {
            labelposY    = posY;
            yLabelWidth  = static_cast<int>(width);
            labelFmtY    = newLabelFmt;
            return *this;
        }

        MainAttributes& setSelectionMode(SelMode mode) { mouseSelMode = mode; return *this; }

    private:
        friend class Graph2D;

        bool minXauto; //autodetect range for X value
        bool maxXauto;
        double minX; //x-range to visualize
        double maxX;

        bool minYauto; //autodetect range for Y value
        bool maxYauto;
        double minY; //y-range to visualize
        double maxY;

        PosLabelX labelposX;
        int xLabelHeight;
        std::shared_ptr<LabelFormatter> labelFmtX;

        PosLabelY labelposY;
        int yLabelWidth;
        std::shared_ptr<LabelFormatter> labelFmtY;

        SelMode mouseSelMode;
    };
    void setAttributes(const MainAttributes& newAttr) { attr = newAttr; Refresh(); }
    MainAttributes getAttributes() const { return attr; }

    std::vector<SelectionBlock> getSelections() const { return oldSel; }
    void setSelections(const std::vector<SelectionBlock>& sel)
    {
        oldSel = sel;
        activeSel.reset();
        Refresh();
    }
    void clearSelection() { oldSel.clear(); Refresh(); }

private:
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseMovement(wxMouseEvent& event);
    void OnMouseLeftUp  (wxMouseEvent& event);
    void OnMouseCaptureLost(wxMouseCaptureLostEvent& event);

    void onPaintEvent(wxPaintEvent& event);
    void onSizeEvent(wxSizeEvent& event) { Refresh(); event.Skip(); }
    void onEraseBackGround(wxEraseEvent& event) {}

    void render(wxDC& dc) const;

    class MouseSelection
    {
    public:
        MouseSelection(wxWindow& wnd, const wxPoint& posDragStart) : wnd_(wnd), posDragStart_(posDragStart), posDragCurrent(posDragStart) { wnd_.CaptureMouse(); }
        ~MouseSelection() { if (wnd_.HasCapture()) wnd_.ReleaseMouse(); }

        wxPoint getStartPos() const { return posDragStart_; }
        wxPoint& refCurrentPos() { return posDragCurrent; }

        SelectionBlock& refSelection() { return selBlock; } //updated in Graph2d::render(): this is fine, since only what's shown is selected!

    private:
        wxWindow& wnd_;
        const wxPoint posDragStart_;
        wxPoint posDragCurrent;
        SelectionBlock selBlock;
    };
    std::vector<SelectionBlock>     oldSel; //applied selections
    std::shared_ptr<MouseSelection> activeSel; //set during mouse selection

    MainAttributes attr; //global attributes

    std::unique_ptr<wxBitmap> doubleBuffer;

    typedef std::vector<std::pair<std::shared_ptr<GraphData>, CurveAttributes>> GraphList;
    GraphList curves_;
};
}

#endif //WX_PLOT_HEADER_2344252459

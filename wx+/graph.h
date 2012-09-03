// **************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under *
// * GNU General Public License: http://www.gnu.org/licenses/gpl.html       *
// * Copyright (C) ZenJu (zenju AT gmx DOT de) - All Rights Reserved        *
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
    m_panelGraph->setAttributes(Graph2D::GraphAttributes().
                                setLabelX(Graph2D::POSLX_BOTTOM, 20, std::make_shared<LabelFormatterTimeElapsed>()).
                                setLabelY(Graph2D::POSLY_RIGHT,  60, std::make_shared<LabelFormatterBytes>()));
    //set graph data
    std::shared_ptr<GraphData> graphDataBytes = ...
	m_panelGraph->setData(graphDataBytes, Graph2D::LineAttributes().setLineWidth(2).setColor(wxColor(0, 192, 0)));
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

//------------------------------------------------------------------------------------------------------------
struct LabelFormatter
{
    virtual ~LabelFormatter() {}

    //determine convenient graph label block size in unit of data: usually some small deviation on "sizeProposed"
    virtual double getOptimalBlockSize(double sizeProposed) const = 0;

    //create human-readable text for x or y-axis position
    virtual wxString formatText(double value, double optimalBlockSize) const = 0;
};

double nextNiceNumber(double blockSize); //round to next number which is convenient to read, e.g. 2.13 -> 2; 2.7 -> 2.5; 7 -> 5

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

    class LineAttributes
    {
    public:
        LineAttributes() : autoColor(true), lineWidth(2) {}

        LineAttributes& setColor(const wxColour& col) { color = col; autoColor = false; return *this; }
        LineAttributes& setLineWidth(size_t width) { lineWidth = static_cast<int>(width); return *this; }

    private:
        friend class Graph2D;

        bool     autoColor;
        wxColour color;
        int      lineWidth;
    };

    void setData(const std::shared_ptr<GraphData>& data, const LineAttributes& attr = LineAttributes());
    void addData(const std::shared_ptr<GraphData>& data, const LineAttributes& attr = LineAttributes());

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

    class GraphAttributes
    {
    public:
        GraphAttributes() :
            minXauto(true),
            maxXauto(true),
            minX(0),
            maxX(0),
            minYauto(true),
            maxYauto(true),
            minY(0),
            maxY(0),
            labelposX(X_LABEL_BOTTOM),
            labelHeightX(25),
            labelFmtX(new DecimalNumberFormatter()),
            labelposY(Y_LABEL_LEFT),
            labelWidthY(60),
            labelFmtY(new DecimalNumberFormatter()),
            mouseSelMode(SELECT_RECTANGLE) {}


        GraphAttributes& setMinX(double newMinX) { minX = newMinX; minXauto = false; return *this; }
        GraphAttributes& setMaxX(double newMaxX) { maxX = newMaxX; maxXauto = false; return *this; }

        GraphAttributes& setMinY(double newMinY) { minY = newMinY; minYauto = false; return *this; }
        GraphAttributes& setMaxY(double newMaxY) { maxY = newMaxY; maxYauto = false; return *this; }

        GraphAttributes& setAutoSize() { minXauto = true; maxXauto = true; minYauto = true; maxYauto = true; return *this; }

        static const std::shared_ptr<LabelFormatter> defaultFormat;

        GraphAttributes& setLabelX(PosLabelX posX, size_t height = 25, const std::shared_ptr<LabelFormatter>& newLabelFmt = defaultFormat)
        {
            labelposX    = posX;
            labelHeightX = static_cast<int>(height);
            labelFmtX    = newLabelFmt;
            return *this;
        }
        GraphAttributes& setLabelY(PosLabelY posY, size_t width = 60, const std::shared_ptr<LabelFormatter>& newLabelFmt = defaultFormat)
        {
            labelposY    = posY;
            labelWidthY  = static_cast<int>(width);
            labelFmtY    = newLabelFmt;
            return *this;
        }

        GraphAttributes& setSelectionMode(SelMode mode) { mouseSelMode = mode; return *this; }

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
        int labelHeightX;
        std::shared_ptr<LabelFormatter> labelFmtX;

        PosLabelY labelposY;
        int labelWidthY;
        std::shared_ptr<LabelFormatter> labelFmtY;

        SelMode mouseSelMode;
    };
    void setAttributes(const GraphAttributes& newAttr) { attr = newAttr; Refresh(); }
    GraphAttributes getAttributes() const { return attr; }


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

    void onPaintEvent(wxPaintEvent& event)
    {
        wxAutoBufferedPaintDC dc(this); //this one happily fucks up for RTL layout by not drawing the first column (x = 0)!
        //wxPaintDC dc(this);
        render(dc);
    }

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

        SelectionBlock& refSelection() { return selBlock; } //set when selection is drawn: this is fine, 'cause only what's shown should be selected!

    private:
        wxWindow& wnd_;
        const wxPoint posDragStart_;
        wxPoint posDragCurrent;
        SelectionBlock selBlock;
    };
    std::vector<SelectionBlock>     oldSel; //applied selections
    std::shared_ptr<MouseSelection> activeSel; //set during mouse selection

    GraphAttributes attr; //global attributes

    typedef std::vector<std::pair<std::shared_ptr<GraphData>, LineAttributes>> GraphList;
    GraphList curves_;
};
}


#endif //WX_PLOT_HEADER_2344252459

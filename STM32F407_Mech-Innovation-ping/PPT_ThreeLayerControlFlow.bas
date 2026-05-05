Option Explicit

' PowerPoint VBA 使用方法：
' 1. Alt + F11 打开 VBA 编辑器
' 2. Insert -> Module
' 3. 粘贴本文件内容
' 4. 运行 CreateThreeLayerControlFlow

Public Sub CreateThreeLayerControlFlow()
    Dim pres As Presentation
    Dim sld As Slide
    Dim sw As Single
    Dim sh As Single

    If Application.Presentations.Count = 0 Then
        Set pres = Application.Presentations.Add
    Else
        Set pres = ActivePresentation
    End If

    pres.PageSetup.SlideWidth = 960
    pres.PageSetup.SlideHeight = 540
    sw = pres.PageSetup.SlideWidth
    sh = pres.PageSetup.SlideHeight

    Set sld = pres.Slides.Add(pres.Slides.Count + 1, ppLayoutBlank)
    sld.FollowMasterBackground = msoFalse
    sld.Background.Fill.ForeColor.RGB = RGB(247, 249, 252)

    AddHeader sld, sw
    DrawControllerCore sld
    DrawLayerArchitecture sld
    DrawSafetyAndLoop sld
    DrawDesignHighlights sld
End Sub

Private Sub AddHeader(ByVal sld As Slide, ByVal sw As Single)
    Dim titleBox As Shape
    Dim subBox As Shape

    Set titleBox = sld.Shapes.AddTextbox(msoTextOrientationHorizontal, 30, 18, sw - 60, 34)
    With titleBox.TextFrame
        .TextRange.Text = "三层洗菜系统电控架构流程"
        .TextRange.ParagraphFormat.Alignment = ppAlignLeft
        With .TextRange.Font
            .Name = "Microsoft YaHei"
            .Size = 25
            .Bold = msoTrue
            .Color.RGB = RGB(22, 34, 55)
        End With
    End With

    Set subBox = sld.Shapes.AddTextbox(msoTextOrientationHorizontal, 32, 52, sw - 64, 22)
    With subBox.TextFrame
        .TextRange.Text = "以状态机为核心，将清洗、切割、称重打包三层动作按节拍自动衔接"
        With .TextRange.Font
            .Name = "Microsoft YaHei"
            .Size = 11
            .Color.RGB = RGB(88, 101, 120)
        End With
    End With
End Sub

Private Sub DrawControllerCore(ByVal sld As Slide)
    AddCoreBox sld, "主控状态机", _
               "启动命令" & vbCrLf & _
               "状态判断" & vbCrLf & _
               "节拍计时" & vbCrLf & _
               "安全停机", _
               38, 116, 120, 270, RGB(44, 88, 165)

    AddArrow sld, 158, 250, 208, 250, RGB(68, 82, 105), 2#

    AddSmallText sld, "统一调度三层执行机构" & vbCrLf & _
                      "避免各层动作互相阻塞", _
                 32, 398, 140, 44, RGB(83, 97, 118)
End Sub

Private Sub DrawLayerArchitecture(ByVal sld As Slide)
    Dim c1 As Long
    Dim c2 As Long
    Dim c3 As Long
    Dim dark1 As Long
    Dim dark2 As Long
    Dim dark3 As Long

    c1 = RGB(222, 237, 255)
    c2 = RGB(255, 238, 217)
    c3 = RGB(224, 244, 229)
    dark1 = RGB(45, 105, 196)
    dark2 = RGB(204, 124, 38)
    dark3 = RGB(43, 139, 83)

    AddLayerBand sld, "第一层控制：清洗与输送", _
                 "接收物料 -> 输送推进 -> 清洗处理 -> 放行到中层", _
                 205, 86, 480, 100, c1, dark1

    AddLayerBand sld, "第二层控制：定位与切割", _
                 "承接物料 -> 定时定位 -> 切割执行 -> 送入称重区", _
                 205, 208, 480, 100, c2, dark2

    AddLayerBand sld, "第三层控制：称重、推送与打包", _
                 "稳定称重 -> 推板送料 -> 打包联动 -> 切断收尾 -> 推板复位", _
                 205, 330, 480, 120, c3, dark3

    AddDownArrow sld, 445, 186, 445, 208, RGB(68, 82, 105)
    AddDownArrow sld, 445, 308, 445, 330, RGB(68, 82, 105)

    AddNode sld, "上层完成", 690, 122, 76, 30, RGB(255, 255, 255), dark1
    AddNode sld, "中层完成", 690, 244, 76, 30, RGB(255, 255, 255), dark2
    AddNode sld, "本轮完成", 690, 382, 76, 30, RGB(255, 255, 255), dark3

    AddArrow sld, 685, 137, 690, 137, RGB(68, 82, 105), 1.5
    AddArrow sld, 685, 259, 690, 259, RGB(68, 82, 105), 1.5
    AddArrow sld, 685, 397, 690, 397, RGB(68, 82, 105), 1.5
End Sub

Private Sub DrawSafetyAndLoop(ByVal sld As Slide)
    Dim loopColor As Long
    Dim safeColor As Long

    loopColor = RGB(91, 116, 150)
    safeColor = RGB(191, 72, 62)

    AddArrow sld, 728, 397, 728, 470, loopColor, 1.6
    AddArrow sld, 728, 470, 222, 470, loopColor, 1.6
    AddArrow sld, 222, 470, 222, 186, loopColor, 1.6
    AddSmallText sld, "循环节拍：" & vbCrLf & "复位后进入下一轮", 360, 452, 150, 40, loopColor

    AddSafetyBox sld, "停机策略", _
                 "普通停止：先停止进料，再等待中层和第三层排空" & vbCrLf & _
                 "急停处理：立即停止全部动作，并回到安全状态", _
                 38, 456, 610, 54, safeColor
End Sub

Private Sub DrawDesignHighlights(ByVal sld As Slide)
    Dim box As Shape
    Dim txt As String

    Set box = sld.Shapes.AddShape(msoShapeRoundedRectangle, 790, 92, 136, 340)
    With box
        .Fill.ForeColor.RGB = RGB(255, 255, 255)
        .Line.ForeColor.RGB = RGB(75, 114, 172)
        .Line.Weight = 1.4
        .Shadow.Visible = msoTrue
        .Shadow.Transparency = 0.82
    End With

    txt = "控制设计重点" & vbCrLf & vbCrLf & _
          "1. 分层控制" & vbCrLf & _
          "   每层职责清晰" & vbCrLf & vbCrLf & _
          "2. 状态机调度" & vbCrLf & _
          "   动作按阶段推进" & vbCrLf & vbCrLf & _
          "3. 非阻塞计时" & vbCrLf & _
          "   主循环持续响应" & vbCrLf & vbCrLf & _
          "4. 协同执行" & vbCrLf & _
          "   推送与打包联动" & vbCrLf & vbCrLf & _
          "5. 安全闭环" & vbCrLf & _
          "   停机与急停分级"

    With box.TextFrame
        .MarginLeft = 10
        .MarginRight = 8
        .MarginTop = 10
        .MarginBottom = 8
        .WordWrap = msoTrue
        .TextRange.Text = txt
        With .TextRange.Font
            .Name = "Microsoft YaHei"
            .Size = 10
            .Color.RGB = RGB(40, 50, 68)
        End With
        .TextRange.Paragraphs(1).Font.Size = 16
        .TextRange.Paragraphs(1).Font.Bold = msoTrue
        .TextRange.Paragraphs(1).Font.Color.RGB = RGB(38, 88, 157)
    End With
End Sub

Private Sub AddCoreBox(ByVal sld As Slide, ByVal titleText As String, ByVal bodyText As String, _
                       ByVal x As Single, ByVal y As Single, ByVal w As Single, ByVal h As Single, _
                       ByVal themeColor As Long)
    Dim shp As Shape

    Set shp = sld.Shapes.AddShape(msoShapeRoundedRectangle, x, y, w, h)
    With shp
        .Fill.ForeColor.RGB = themeColor
        .Line.Visible = msoFalse
        With .TextFrame
            .MarginLeft = 10
            .MarginRight = 10
            .MarginTop = 14
            .MarginBottom = 10
            .VerticalAnchor = msoAnchorMiddle
            .TextRange.Text = titleText & vbCrLf & vbCrLf & bodyText
            .TextRange.ParagraphFormat.Alignment = ppAlignCenter
            With .TextRange.Font
                .Name = "Microsoft YaHei"
                .Size = 13
                .Bold = msoTrue
                .Color.RGB = RGB(255, 255, 255)
            End With
            .TextRange.Paragraphs(1).Font.Size = 18
        End With
    End With
End Sub

Private Sub AddLayerBand(ByVal sld As Slide, ByVal titleText As String, ByVal flowText As String, _
                         ByVal x As Single, ByVal y As Single, ByVal w As Single, ByVal h As Single, _
                         ByVal fillColor As Long, ByVal lineColor As Long)
    Dim band As Shape
    Dim titleBox As Shape
    Dim flowBox As Shape

    Set band = sld.Shapes.AddShape(msoShapeRoundedRectangle, x, y, w, h)
    With band
        .Fill.ForeColor.RGB = fillColor
        .Line.ForeColor.RGB = lineColor
        .Line.Weight = 1.3
    End With

    Set titleBox = sld.Shapes.AddShape(msoShapeRoundedRectangle, x + 14, y + 15, 150, 34)
    With titleBox
        .Fill.ForeColor.RGB = lineColor
        .Line.Visible = msoFalse
        With .TextFrame
            .MarginLeft = 6
            .MarginRight = 6
            .MarginTop = 3
            .MarginBottom = 3
            .VerticalAnchor = msoAnchorMiddle
            .TextRange.Text = titleText
            .TextRange.ParagraphFormat.Alignment = ppAlignCenter
            With .TextRange.Font
                .Name = "Microsoft YaHei"
                .Size = 11
                .Bold = msoTrue
                .Color.RGB = RGB(255, 255, 255)
            End With
        End With
    End With

    Set flowBox = sld.Shapes.AddShape(msoShapeRoundedRectangle, x + 182, y + 18, w - 206, h - 36)
    With flowBox
        .Fill.ForeColor.RGB = RGB(255, 255, 255)
        .Line.ForeColor.RGB = lineColor
        .Line.Transparency = 0.45
        With .TextFrame
            .MarginLeft = 12
            .MarginRight = 12
            .MarginTop = 6
            .MarginBottom = 6
            .VerticalAnchor = msoAnchorMiddle
            .WordWrap = msoTrue
            .TextRange.Text = flowText
            .TextRange.ParagraphFormat.Alignment = ppAlignCenter
            With .TextRange.Font
                .Name = "Microsoft YaHei"
                .Size = 12
                .Bold = msoTrue
                .Color.RGB = RGB(38, 48, 64)
            End With
        End With
    End With
End Sub

Private Sub AddNode(ByVal sld As Slide, ByVal textValue As String, _
                    ByVal x As Single, ByVal y As Single, ByVal w As Single, ByVal h As Single, _
                    ByVal fillColor As Long, ByVal lineColor As Long)
    Dim shp As Shape

    Set shp = sld.Shapes.AddShape(msoShapeRoundedRectangle, x, y, w, h)
    With shp
        .Fill.ForeColor.RGB = fillColor
        .Line.ForeColor.RGB = lineColor
        .Line.Weight = 1.1
        With .TextFrame
            .MarginLeft = 4
            .MarginRight = 4
            .MarginTop = 2
            .MarginBottom = 2
            .VerticalAnchor = msoAnchorMiddle
            .TextRange.Text = textValue
            .TextRange.ParagraphFormat.Alignment = ppAlignCenter
            With .TextRange.Font
                .Name = "Microsoft YaHei"
                .Size = 9.5
                .Bold = msoTrue
                .Color.RGB = RGB(40, 50, 68)
            End With
        End With
    End With
End Sub

Private Sub AddSafetyBox(ByVal sld As Slide, ByVal titleText As String, ByVal bodyText As String, _
                         ByVal x As Single, ByVal y As Single, ByVal w As Single, ByVal h As Single, _
                         ByVal lineColor As Long)
    Dim shp As Shape

    Set shp = sld.Shapes.AddShape(msoShapeRoundedRectangle, x, y, w, h)
    With shp
        .Fill.ForeColor.RGB = RGB(255, 239, 236)
        .Line.ForeColor.RGB = lineColor
        .Line.Weight = 1.2
        With .TextFrame
            .MarginLeft = 12
            .MarginRight = 10
            .MarginTop = 5
            .MarginBottom = 5
            .VerticalAnchor = msoAnchorMiddle
            .TextRange.Text = titleText & "：" & bodyText
            With .TextRange.Font
                .Name = "Microsoft YaHei"
                .Size = 10
                .Color.RGB = RGB(92, 46, 43)
            End With
            .TextRange.Characters(1, Len(titleText) + 1).Font.Bold = msoTrue
        End With
    End With
End Sub

Private Sub AddSmallText(ByVal sld As Slide, ByVal textValue As String, _
                         ByVal x As Single, ByVal y As Single, ByVal w As Single, ByVal h As Single, _
                         ByVal fontColor As Long)
    Dim shp As Shape

    Set shp = sld.Shapes.AddTextbox(msoTextOrientationHorizontal, x, y, w, h)
    With shp.TextFrame
        .MarginLeft = 2
        .MarginRight = 2
        .MarginTop = 2
        .MarginBottom = 2
        .WordWrap = msoTrue
        .TextRange.Text = textValue
        With .TextRange.Font
            .Name = "Microsoft YaHei"
            .Size = 9.5
            .Color.RGB = fontColor
        End With
    End With
End Sub

Private Sub AddArrow(ByVal sld As Slide, ByVal x1 As Single, ByVal y1 As Single, _
                     ByVal x2 As Single, ByVal y2 As Single, ByVal lineColor As Long, _
                     ByVal weightValue As Single)
    Dim conn As Shape

    Set conn = sld.Shapes.AddConnector(msoConnectorStraight, x1, y1, x2, y2)
    With conn.Line
        .ForeColor.RGB = lineColor
        .Weight = weightValue
        .EndArrowheadStyle = msoArrowheadTriangle
    End With
End Sub

Private Sub AddDownArrow(ByVal sld As Slide, ByVal x1 As Single, ByVal y1 As Single, _
                         ByVal x2 As Single, ByVal y2 As Single, ByVal lineColor As Long)
    AddArrow sld, x1, y1, x2, y2, lineColor, 1.7
End Sub

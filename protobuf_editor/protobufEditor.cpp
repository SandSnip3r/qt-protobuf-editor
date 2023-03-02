#include "protobufEditor.hpp"
#include "messageTypeWidget.hpp"

#include "proto/test.pb.h"

#include <QScrollArea>
#include <QVBoxLayout>

#include <iostream>

namespace pb = google::protobuf;

ProtobufEditor::ProtobufEditor(QWidget *parent) : QWidget{parent} {
  // Create a layout for this widget
  QVBoxLayout *layout = new QVBoxLayout(this);

  // Since the protobuf message could be arbitrarily large, this view will be scrollable
  QScrollArea *scrollArea = new QScrollArea;

  // Get the descriptor of the message that we want to be able to edit. This descriptor is how the widget knows what UI elements to build
  const pb::Descriptor *desc = proto::test::Test::GetDescriptor();

  // Construct a widget to edit a protobuf message, pass the descriptor
  protobuf_editor::MessageTypeWidget *w = new protobuf_editor::MessageTypeWidget(desc);

  // Make this message editing widget the main widget of the scroll area
  scrollArea->setWidget(w);
  layout->addWidget(scrollArea);

  // Allocate a message for the widget to reference on the heap. Do not allocate it, since the widget will always reference this
  proto::test::Test *testMsg = new proto::test::Test;
  w->setMessage(testMsg);

  // When any edits are made in the widget, this signal will be emitted
  connect(w, &protobuf_editor::ProtobufFieldWidget::messageUpdated, [testMsg]{
    std::cout << "Message updated!" << std::endl;
    std::cout << testMsg->DebugString() << std::endl;
  });
}

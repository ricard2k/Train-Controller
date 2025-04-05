#pragma once

#include "IPage.h"
#include "config.h"
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <vector>
#include <functional>
#include "ListItem.h"

class DialogListPage : public IPage {
public:
    DialogListPage(TFT_eSPI* tft, const String& title, const std::vector<ListItem>& items,
                   std::function<void(bool accepted, ListItem selected)> callback);

    void handleInput() override;
    void draw() override;
    TFT_eSPI* getDisplay() const override;

private:
    void drawItems();
    void drawButtons();
    void moveSelection(int delta);

    TFT_eSPI* tft;
    String title;
    std::vector<ListItem> items;
    std::function<void(bool, ListItem)> callback;

    int selectedIndex = 0;
    boolean focusOnButtons = false;
    int selectedButton = 0; // 0 = OK, 1 = Cancel

    int listTopY = 40;
    int itemHeight = 24;
};



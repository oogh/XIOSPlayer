//
// Created by Oogh on 2020/12/18.
//

#ifndef XVIEWOBSERVER_HPP
#define XVIEWOBSERVER_HPP

#include <vector>

class XImage;

struct XRenderObserver {
    virtual void update(const std::vector<std::shared_ptr<XImage>>& images) = 0;
};

#endif // XVIEWOBSERVER_HPP

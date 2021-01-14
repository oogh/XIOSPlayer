//
//  XWeakProxy.h
//  XRender
//
//  Created by Oogh on 2020/04/17.
//  Copyright © 2020 Oogh. All rights reserved.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface XWeakProxy : NSProxy

/**
 The proxy target.
 */
@property (nonatomic, weak, readonly) id target;
 
/**
 Creates a new weak proxy for target.
  
 @param target Target object.
  
 @return A new proxy object.
 */
- (instancetype)initWithTarget:(id)target;
 
/**
 Creates a new weak proxy for target.
  
 @param target Target object.
  
 @return A new proxy object.
 */
+ (instancetype)proxyWithTarget:(id)target;

@end

NS_ASSUME_NONNULL_END

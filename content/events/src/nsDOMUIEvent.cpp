/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Steve Clark (buster@netscape.com)
 *   Ilya Konstantinov (mozilla-code@future.shiny.co.il)
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "base/basictypes.h"
#include "IPC/IPCMessageUtils.h"
#include "nsCOMPtr.h"
#include "nsDOMUIEvent.h"
#include "nsIPresShell.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIDOMWindow.h"
#include "nsIDOMNode.h"
#include "nsIContent.h"
#include "nsContentUtils.h"
#include "nsEventStateManager.h"
#include "nsIFrame.h"
#include "nsIScrollableFrame.h"
#include "DictionaryHelpers.h"
#include "mozilla/Util.h"
#include "mozilla/Assertions.h"

using namespace mozilla;

nsDOMUIEvent::nsDOMUIEvent(nsPresContext* aPresContext, nsGUIEvent* aEvent)
  : nsDOMEvent(aPresContext, aEvent ?
               static_cast<nsEvent *>(aEvent) :
               static_cast<nsEvent *>(new nsUIEvent(false, 0, 0)))
  , mClientPoint(0, 0), mLayerPoint(0, 0), mPagePoint(0, 0)
  , mIsPointerLocked(nsEventStateManager::sIsPointerLocked)
  , mLastScreenPoint(nsEventStateManager::sLastScreenPoint)
  , mLastClientPoint(nsEventStateManager::sLastClientPoint)
{
  if (aEvent) {
    mEventIsInternal = false;
  }
  else {
    mEventIsInternal = true;
    mEvent->time = PR_Now();
  }
  
  // Fill mDetail and mView according to the mEvent (widget-generated
  // event) we've got
  switch(mEvent->eventStructType)
  {
    case NS_UI_EVENT:
    {
      nsUIEvent *event = static_cast<nsUIEvent*>(mEvent);
      mDetail = event->detail;
      break;
    }

    case NS_SCROLLPORT_EVENT:
    {
      nsScrollPortEvent* scrollEvent = static_cast<nsScrollPortEvent*>(mEvent);
      mDetail = (PRInt32)scrollEvent->orient;
      break;
    }

    default:
      mDetail = 0;
      break;
  }

  mView = nsnull;
  if (mPresContext)
  {
    nsCOMPtr<nsISupports> container = mPresContext->GetContainer();
    if (container)
    {
       nsCOMPtr<nsIDOMWindow> window = do_GetInterface(container);
       if (window)
          mView = do_QueryInterface(window);
    }
  }
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMUIEvent)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMUIEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mView)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMUIEvent, nsDOMEvent)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mView)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsDOMUIEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMUIEvent, nsDOMEvent)

DOMCI_DATA(UIEvent, nsDOMUIEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMUIEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMUIEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(UIEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

nsIntPoint
nsDOMUIEvent::GetMovementPoint()
{
  if (!mEvent ||
       (mEvent->eventStructType != NS_MOUSE_EVENT &&
        mEvent->eventStructType != NS_POPUP_EVENT &&
        mEvent->eventStructType != NS_MOUSE_SCROLL_EVENT &&
        mEvent->eventStructType != NS_MOZTOUCH_EVENT &&
        mEvent->eventStructType != NS_DRAG_EVENT &&
        mEvent->eventStructType != NS_SIMPLE_GESTURE_EVENT)) {
    return nsIntPoint(0, 0);
  }

  if (!((nsGUIEvent*)mEvent)->widget) {
    return mEvent->lastRefPoint;
  }

  // Calculate the delta between the previous screen point and the current one.
  nsIntPoint currentPoint = CalculateScreenPoint(mPresContext, mEvent);

  // Adjust previous event's refPoint so it compares to current screenX, screenY
  nsIntPoint offset = mEvent->lastRefPoint +
    ((nsGUIEvent*)mEvent)->widget->WidgetToScreenOffset();
  nscoord factor = mPresContext->DeviceContext()->UnscaledAppUnitsPerDevPixel();
  nsIntPoint lastPoint = nsIntPoint(nsPresContext::AppUnitsToIntCSSPixels(offset.x * factor),
                                    nsPresContext::AppUnitsToIntCSSPixels(offset.y * factor));

  return currentPoint - lastPoint;
}

nsIntPoint
nsDOMUIEvent::GetScreenPoint()
{
  if (mIsPointerLocked) {
    return mLastScreenPoint;
  }

  return CalculateScreenPoint(mPresContext, mEvent);
}

nsIntPoint
nsDOMUIEvent::GetClientPoint()
{
  if (mIsPointerLocked) {
    return mLastClientPoint;
  }

  return CalculateClientPoint(mPresContext, mEvent, &mClientPoint);
}

NS_IMETHODIMP
nsDOMUIEvent::GetView(nsIDOMWindow** aView)
{
  *aView = mView;
  NS_IF_ADDREF(*aView);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetDetail(PRInt32* aDetail)
{
  *aDetail = mDetail;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::InitUIEvent(const nsAString& typeArg,
                          bool canBubbleArg,
                          bool cancelableArg,
                          nsIDOMWindow* viewArg,
                          PRInt32 detailArg)
{
  nsresult rv = nsDOMEvent::InitEvent(typeArg, canBubbleArg, cancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mDetail = detailArg;
  mView = viewArg;

  return NS_OK;
}

nsresult
nsDOMUIEvent::InitFromCtor(const nsAString& aType,
                           JSContext* aCx, jsval* aVal)
{
  mozilla::dom::UIEventInit d;
  nsresult rv = d.Init(aCx, aVal);
  NS_ENSURE_SUCCESS(rv, rv);
  return InitUIEvent(aType, d.bubbles, d.cancelable, d.view, d.detail);
}

// ---- nsDOMNSUIEvent implementation -------------------
nsIntPoint
nsDOMUIEvent::GetPagePoint()
{
  if (mPrivateDataDuplicated) {
    return mPagePoint;
  }

  nsIntPoint pagePoint = GetClientPoint();

  // If there is some scrolling, add scroll info to client point.
  if (mPresContext && mPresContext->GetPresShell()) {
    nsIPresShell* shell = mPresContext->GetPresShell();
    nsIScrollableFrame* scrollframe = shell->GetRootScrollFrameAsScrollable();
    if (scrollframe) {
      nsPoint pt = scrollframe->GetScrollPosition();
      pagePoint += nsIntPoint(nsPresContext::AppUnitsToIntCSSPixels(pt.x),
                              nsPresContext::AppUnitsToIntCSSPixels(pt.y));
    }
  }

  return pagePoint;
}

NS_IMETHODIMP
nsDOMUIEvent::GetPageX(PRInt32* aPageX)
{
  NS_ENSURE_ARG_POINTER(aPageX);
  if (mPrivateDataDuplicated) {
    *aPageX = mPagePoint.x;
  } else {
    *aPageX = nsDOMEvent::GetPageCoords(mPresContext,
                                        mEvent,
                                        mEvent->refPoint,
                                        mClientPoint).x;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetPageY(PRInt32* aPageY)
{
  NS_ENSURE_ARG_POINTER(aPageY);
  if (mPrivateDataDuplicated) {
    *aPageY = mPagePoint.y;
  } else {
    *aPageY = nsDOMEvent::GetPageCoords(mPresContext,
                                        mEvent,
                                        mEvent->refPoint,
                                        mClientPoint).y;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetWhich(PRUint32* aWhich)
{
  return Which(aWhich);
}

NS_IMETHODIMP
nsDOMUIEvent::GetRangeParent(nsIDOMNode** aRangeParent)
{
  NS_ENSURE_ARG_POINTER(aRangeParent);
  nsIFrame* targetFrame = nsnull;

  if (mPresContext) {
    targetFrame = mPresContext->EventStateManager()->GetEventTarget();
  }

  *aRangeParent = nsnull;

  if (targetFrame) {
    nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(mEvent,
                                                              targetFrame);
    nsCOMPtr<nsIContent> parent = targetFrame->GetContentOffsetsFromPoint(pt).content;
    if (parent) {
      if (parent->IsInNativeAnonymousSubtree() &&
          !nsContentUtils::CanAccessNativeAnon()) {
        return NS_OK;
      }
      return CallQueryInterface(parent, aRangeParent);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetRangeOffset(PRInt32* aRangeOffset)
{
  NS_ENSURE_ARG_POINTER(aRangeOffset);
  nsIFrame* targetFrame = nsnull;

  if (mPresContext) {
    targetFrame = mPresContext->EventStateManager()->GetEventTarget();
  }

  if (targetFrame) {
    nsPoint pt = nsLayoutUtils::GetEventCoordinatesRelativeTo(mEvent,
                                                              targetFrame);
    *aRangeOffset = targetFrame->GetContentOffsetsFromPoint(pt).offset;
    return NS_OK;
  }
  *aRangeOffset = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetCancelBubble(bool* aCancelBubble)
{
  NS_ENSURE_ARG_POINTER(aCancelBubble);
  *aCancelBubble =
    (mEvent->flags & NS_EVENT_FLAG_STOP_DISPATCH) ? true : false;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::SetCancelBubble(bool aCancelBubble)
{
  if (aCancelBubble) {
    mEvent->flags |= NS_EVENT_FLAG_STOP_DISPATCH;
  } else {
    mEvent->flags &= ~NS_EVENT_FLAG_STOP_DISPATCH;
  }
  return NS_OK;
}

nsIntPoint
nsDOMUIEvent::GetLayerPoint()
{
  if (!mEvent ||
      (mEvent->eventStructType != NS_MOUSE_EVENT &&
       mEvent->eventStructType != NS_POPUP_EVENT &&
       mEvent->eventStructType != NS_MOUSE_SCROLL_EVENT &&
       mEvent->eventStructType != NS_MOZTOUCH_EVENT &&
       mEvent->eventStructType != NS_TOUCH_EVENT &&
       mEvent->eventStructType != NS_DRAG_EVENT &&
       mEvent->eventStructType != NS_SIMPLE_GESTURE_EVENT) ||
      !mPresContext ||
      mEventIsInternal) {
    return mLayerPoint;
  }
  // XXX I'm not really sure this is correct; it's my best shot, though
  nsIFrame* targetFrame = mPresContext->EventStateManager()->GetEventTarget();
  if (!targetFrame)
    return mLayerPoint;
  nsIFrame* layer = nsLayoutUtils::GetClosestLayer(targetFrame);
  nsPoint pt(nsLayoutUtils::GetEventCoordinatesRelativeTo(mEvent, layer));
  return nsIntPoint(nsPresContext::AppUnitsToIntCSSPixels(pt.x),
                    nsPresContext::AppUnitsToIntCSSPixels(pt.y));
}

NS_IMETHODIMP
nsDOMUIEvent::GetLayerX(PRInt32* aLayerX)
{
  NS_ENSURE_ARG_POINTER(aLayerX);
  *aLayerX = GetLayerPoint().x;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetLayerY(PRInt32* aLayerY)
{
  NS_ENSURE_ARG_POINTER(aLayerY);
  *aLayerY = GetLayerPoint().y;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMUIEvent::GetIsChar(bool* aIsChar)
{
  switch(mEvent->eventStructType)
  {
    case NS_KEY_EVENT:
      *aIsChar = ((nsKeyEvent*)mEvent)->isChar;
      return NS_OK;
    case NS_TEXT_EVENT:
      *aIsChar = ((nsTextEvent*)mEvent)->isChar;
      return NS_OK;
    default:
      *aIsChar = false;
      return NS_OK;
  }
}

NS_METHOD
nsDOMUIEvent::DuplicatePrivateData()
{
  mClientPoint = nsDOMEvent::GetClientCoords(mPresContext,
                                             mEvent,
                                             mEvent->refPoint,
                                             mClientPoint);
  mLayerPoint = GetLayerPoint();
  mPagePoint = nsDOMEvent::GetPageCoords(mPresContext,
                                         mEvent,
                                         mEvent->refPoint,
                                         mClientPoint);
  // GetScreenPoint converts mEvent->refPoint to right coordinates.
  nsIntPoint screenPoint = nsDOMEvent::GetScreenCoords(mPresContext,
                                                       mEvent,
                                                       mEvent->refPoint);
  nsresult rv = nsDOMEvent::DuplicatePrivateData();
  if (NS_SUCCEEDED(rv)) {
    mEvent->refPoint = screenPoint;
  }
  return rv;
}

void
nsDOMUIEvent::Serialize(IPC::Message* aMsg, bool aSerializeInterfaceType)
{
  if (aSerializeInterfaceType) {
    IPC::WriteParam(aMsg, NS_LITERAL_STRING("uievent"));
  }

  nsDOMEvent::Serialize(aMsg, false);

  PRInt32 detail = 0;
  GetDetail(&detail);
  IPC::WriteParam(aMsg, detail);
}

bool
nsDOMUIEvent::Deserialize(const IPC::Message* aMsg, void** aIter)
{
  NS_ENSURE_TRUE(nsDOMEvent::Deserialize(aMsg, aIter), false);
  NS_ENSURE_TRUE(IPC::ReadParam(aMsg, aIter, &mDetail), false);
  return true;
}

// XXX Following struct and array are used only in
//     nsDOMUIEvent::ComputeModifierState(), but if we define them in it,
//     we fail to build on Mac at calling mozilla::ArrayLength().
struct nsModifierPair
{
  mozilla::widget::Modifier modifier;
  const char* name;
};
static const nsModifierPair kPairs[] = {
  { widget::MODIFIER_ALT,        NS_DOM_KEYNAME_ALT },
  { widget::MODIFIER_ALTGRAPH,   NS_DOM_KEYNAME_ALTGRAPH },
  { widget::MODIFIER_CAPSLOCK,   NS_DOM_KEYNAME_CAPSLOCK },
  { widget::MODIFIER_CONTROL,    NS_DOM_KEYNAME_CONTROL },
  { widget::MODIFIER_FN,         NS_DOM_KEYNAME_FN },
  { widget::MODIFIER_META,       NS_DOM_KEYNAME_META },
  { widget::MODIFIER_NUMLOCK,    NS_DOM_KEYNAME_NUMLOCK },
  { widget::MODIFIER_SCROLL,     NS_DOM_KEYNAME_SCROLL },
  { widget::MODIFIER_SHIFT,      NS_DOM_KEYNAME_SHIFT },
  { widget::MODIFIER_SYMBOLLOCK, NS_DOM_KEYNAME_SYMBOLLOCK },
  { widget::MODIFIER_WIN,        NS_DOM_KEYNAME_WIN }
};

/* static */
mozilla::widget::Modifiers
nsDOMUIEvent::ComputeModifierState(const nsAString& aModifiersList)
{
  if (aModifiersList.IsEmpty()) {
    return 0;
  }

  // Be careful about the performance.  If aModifiersList is too long,
  // parsing it needs too long time.
  // XXX Should we abort if aModifiersList is too long?

  Modifiers modifiers = 0;

  nsAString::const_iterator listStart, listEnd;
  aModifiersList.BeginReading(listStart);
  aModifiersList.EndReading(listEnd);

  for (PRUint32 i = 0; i < mozilla::ArrayLength(kPairs); i++) {
    nsAString::const_iterator start(listStart), end(listEnd);
    if (!FindInReadable(NS_ConvertASCIItoUTF16(kPairs[i].name), start, end)) {
      continue;
    }

    if ((start != listStart && !NS_IsAsciiWhitespace(*(--start))) ||
        (end != listEnd && !NS_IsAsciiWhitespace(*(end)))) {
      continue;
    }
    modifiers |= kPairs[i].modifier;
  }

  return modifiers;
}

bool
nsDOMUIEvent::GetModifierStateInternal(const nsAString& aKey)
{
  mozilla::widget::Modifiers modifiers = 0;
  switch(mEvent->eventStructType) {
    case NS_MOUSE_EVENT:
    case NS_MOUSE_SCROLL_EVENT:
    case NS_DRAG_EVENT:
    case NS_SIMPLE_GESTURE_EVENT:
    case NS_MOZTOUCH_EVENT:
      modifiers = static_cast<nsMouseEvent_base*>(mEvent)->modifiers;
      break;
    default:
      MOZ_NOT_REACHED("There is no space to store the modifiers");
      return false;
  }

  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_SHIFT)) {
    return static_cast<nsInputEvent*>(mEvent)->isShift;
  }
  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_CONTROL)) {
    return static_cast<nsInputEvent*>(mEvent)->isControl;
  }
  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_META)) {
    return static_cast<nsInputEvent*>(mEvent)->isMeta;
  }
  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_ALT)) {
    return static_cast<nsInputEvent*>(mEvent)->isAlt;
  }

  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_ALTGRAPH)) {
    return (modifiers & widget::MODIFIER_ALTGRAPH) != 0;
  }
  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_WIN)) {
    return (modifiers & widget::MODIFIER_WIN) != 0;
  }


  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_CAPSLOCK)) {
    return (modifiers & widget::MODIFIER_CAPSLOCK) != 0;
  }
  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_NUMLOCK)) {
    return (modifiers & widget::MODIFIER_NUMLOCK) != 0;
  }

  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_FN)) {
    return (modifiers & widget::MODIFIER_FN) != 0;
  }
  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_SCROLL)) {
    return (modifiers & widget::MODIFIER_SCROLL) != 0;
  }
  if (aKey.EqualsLiteral(NS_DOM_KEYNAME_SYMBOLLOCK)) {
    return (modifiers & widget::MODIFIER_SYMBOLLOCK) != 0;
  }
  return false;
}


nsresult NS_NewDOMUIEvent(nsIDOMEvent** aInstancePtrResult,
                          nsPresContext* aPresContext,
                          nsGUIEvent *aEvent) 
{
  nsDOMUIEvent* it = new nsDOMUIEvent(aPresContext, aEvent);
  return CallQueryInterface(it, aInstancePtrResult);
}

/*
 * Common Public Attribution License Version 1.0. 
 * 
 * The contents of this file are subject to the Common Public Attribution 
 * License Version 1.0 (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License 
 * at http://www.xTuple.com/CPAL.  The License is based on the Mozilla 
 * Public License Version 1.1 but Sections 14 and 15 have been added to 
 * cover use of software over a computer network and provide for limited 
 * attribution for the Original Developer. In addition, Exhibit A has 
 * been modified to be consistent with Exhibit B.
 * 
 * Software distributed under the License is distributed on an "AS IS" 
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See 
 * the License for the specific language governing rights and limitations 
 * under the License. 
 * 
 * The Original Code is xTuple ERP: PostBooks Edition 
 * 
 * The Original Developer is not the Initial Developer and is __________. 
 * If left blank, the Original Developer is the Initial Developer. 
 * The Initial Developer of the Original Code is OpenMFG, LLC, 
 * d/b/a xTuple. All portions of the code written by xTuple are Copyright 
 * (c) 1999-2008 OpenMFG, LLC, d/b/a xTuple. All Rights Reserved. 
 * 
 * Contributor(s): ______________________.
 * 
 * Alternatively, the contents of this file may be used under the terms 
 * of the xTuple End-User License Agreeement (the xTuple License), in which 
 * case the provisions of the xTuple License are applicable instead of 
 * those above.  If you wish to allow use of your version of this file only 
 * under the terms of the xTuple License and not to allow others to use 
 * your version of this file under the CPAL, indicate your decision by 
 * deleting the provisions above and replace them with the notice and other 
 * provisions required by the xTuple License. If you do not delete the 
 * provisions above, a recipient may use your version of this file under 
 * either the CPAL or the xTuple License.
 * 
 * EXHIBIT B.  Attribution Information
 * 
 * Attribution Copyright Notice: 
 * Copyright (c) 1999-2008 by OpenMFG, LLC, d/b/a xTuple
 * 
 * Attribution Phrase: 
 * Powered by xTuple ERP: PostBooks Edition
 * 
 * Attribution URL: www.xtuple.org 
 * (to be included in the "Community" menu of the application if possible)
 * 
 * Graphic Image as provided in the Covered Code, if any. 
 * (online at www.xtuple.com/poweredby)
 * 
 * Display of Attribution Information is required in Larger Works which 
 * are defined in the CPAL as a work which combines Covered Code or 
 * portions thereof with code not governed by the terms of the CPAL.
 */

#include "voidChecks.h"

#include <QMessageBox>
#include <QSqlError>
#include <QVariant>

#include <openreports.h>
#include <parameter.h>

#include "guiclient.h"
#include "storedProcErrorLookup.h"

voidChecks::voidChecks(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
    : XDialog(parent, name, modal, fl)
{
  setupUi(this);

  connect(_bankaccnt, SIGNAL(newID(int)), this, SLOT(sHandleBankAccount(int)));
  connect(_void,      SIGNAL(clicked()),  this, SLOT(sVoid()));

  _bankaccnt->setType(XComboBox::APBankAccounts);
  
  _numberOfChecks->setPrecision(0);
}

voidChecks::~voidChecks()
{
  // no need to delete child widgets, Qt does it all for us
}

void voidChecks::languageChange()
{
  retranslateUi(this);
}

void voidChecks::sVoid()
{
  q.prepare("SELECT checkhead_id, checkhead_number,"
	    "       voidCheck(checkhead_id) AS result"
            "  FROM checkhead"
            " WHERE ((NOT checkhead_posted)"
            "   AND  (NOT checkhead_replaced)"
            "   AND  (NOT checkhead_deleted)"
            "   AND  (NOT checkhead_void)"
            "   AND  (checkhead_bankaccnt_id=:bankaccnt_id))");
  q.bindValue(":bankaccnt_id", _bankaccnt->id());
  q.exec();
  while (q.next())
  {
    int result = q.value("result").toInt();
    if (result < 0)
      systemError(this,
		  tr("Check %1").arg(q.value("checkhead_number").toString()) +
		  "\n" + storedProcErrorLookup("voidCheck", result),
		  __FILE__, __LINE__);
    else if(_issueReplacements->isChecked())
    {
      XSqlQuery rplc;
      rplc.prepare("SELECT replaceVoidedCheck(:checkhead_id) AS result;");
      while(q.next())
      {
        rplc.bindValue(":checkhead_id", q.value("checkhead_id").toInt());
        rplc.exec();
      }
    }
    omfgThis->sChecksUpdated(_bankaccnt->id(), -1, TRUE);
  }
  if (q.lastError().type() != QSqlError::NoError)
  {
    systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
    return;
  }

  accept();
}

void voidChecks::sHandleBankAccount(int pBankaccntid)
{
  if (pBankaccntid == -1)
  {
    _checkNumber->clear();
    _numberOfChecks->clear();
  }
  else
  {
    q.prepare( "SELECT TEXT(MIN(checkhead_number)) as checknumber,"
               "       TEXT(COUNT(*)) AS numofchecks "
               "  FROM checkhead "
               " WHERE ( (NOT checkhead_void)"
               "   AND   (NOT checkhead_posted)"
               "   AND   (NOT checkhead_replaced)"
               "   AND   (NOT checkhead_deleted)"
               "   AND   (checkhead_bankaccnt_id=:bankaccnt_id) );" );
    q.bindValue(":bankaccnt_id", pBankaccntid);
    q.exec();
    if (q.first())
    {
      _checkNumber->setText(q.value("checknumber").toString());
      _numberOfChecks->setText(q.value("numofchecks").toString());
    }
    else if (q.lastError().type() != QSqlError::NoError)
    {
      systemError(this, q.lastError().databaseText(), __FILE__, __LINE__);
      return;
    }
  }
}

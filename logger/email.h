#ifndef EMAIL_H
#define EMAIL_H

#include <string>
#include <vector>
#include "pthread.h"

#ifdef _DEBUG
#include <iostream>
#endif

#define  EMAIL_ERROR_RECIPIENT "Recipient List is Empty"
#define  EMAIL_ERROR_MESSAGE   "Transmit  Message is Empty"
#define  EMAIL_ERROR_FAILD     "Failed to invoke sendmail"
#define  EMAIL_ERROR_UNKNOWN   "Unknoun Error"

typedef std::vector<std::string>m_emails;
typedef std::vector<char*> vmsg_box;

class TEMail
{
public       :
    explicit TEMail(m_emails m, vmsg_box);
    ~TEMail();
    void    send_mail();
 private:
    m_emails        emails;
    vmsg_box        transmit_msg;
};

#endif // EMAIL_H

#include "email.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

TEMail::TEMail(m_emails m, vmsg_box msg)
{
    emails = m;
    transmit_msg.assign(msg.begin(), msg.end());
}

TEMail::~TEMail()
{
}


void TEMail::send_mail()
{
    int   email_error    = 0;
    char *s              = NULL;

    if (emails.empty() || transmit_msg.empty())
    {
        email_error = emails.empty() ? 1 : 2;
        //emit finished(email_error);

        return;
    }


    for(m_emails::iterator it = emails.begin(); it != emails.end(); it++)
    {
        /* sudo apt-get install sendmail */
        FILE *mailpipe = popen("/usr/lib/sendmail -t", "w");
        if (mailpipe != NULL)
        {
            s = strdup((*it).c_str());
            fprintf(mailpipe, "To: %s\n", s);
            fprintf(mailpipe, "From: %s\n", "Logger");
            fprintf(mailpipe, "Subject: %s\n\n", "Logger Error Notification");

            for (int i = 0; i < (int)transmit_msg.size(); i++) fprintf(mailpipe, "%s.\n", transmit_msg.at(i));

            //write_len  = fwrite(msg,   1, msg_len, mailpipe);
            //write_len += fwrite(".\n", 1, 2,       mailpipe);

            try        { pclose(mailpipe);}
            catch (...){ email_error = 4; }

            free(s); s = NULL;
         }
         else email_error = 3;

        if (email_error) break;

    }

    for (vmsg_box::iterator it = transmit_msg.begin(); it != transmit_msg.end(); it++) free(*it);
    transmit_msg.clear();
}


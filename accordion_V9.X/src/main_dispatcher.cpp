/*
 * main_dispatcher.cpp
 *
 *  Created on: 08.02.2016
 *      Author: irina
 */
#include "main_dispatcher.h"
#include <arpa/inet.h>
#include "config.h"
#include "playlist.h"
#include "handler_proc.h"
#include "log.h"

/* ---------------------------------------------------*/
std::string get_info()
{
	std::string str = "Hello Vlasov Alexey!";
	return str;
}
void cleaner(tconfig* cf, TPlaylist* pl, Tparameters *lg)
{
	if (cf) { delete cf;  cf  = NULL; }
	if (pl) { delete pl;  pl  = NULL; }
	if (lg) { delete lg;  lg  = NULL; }
	logger::delete_log();
}

/* ---------------------------------------------------*/

int dispatcher(char** argv, int argc)
{

   std::ofstream	outfile;
//   TPlaylist   *pPlaylist  = NULL;
   pConfig  			    = NULL;
   pParameters			    = NULL;

    pConfig   = new tconfig(argv, argc);
    bool flag_working  = pConfig && (pConfig->get_cfg_error() == 0);

    if (flag_working)
    {
    	if (pConfig->get_deamon())
    	{
			if( common::BecomeDaemonProcess() < 0)
			{
				std::cout << "Failed to become daemon process\n";
	        	        std::cout.flush();
				return EXIT_FAILURE;
			}
    	}

        logger::message.clear();

    	logger::ptr_log   = new logger::TInfoLog(pConfig->get_logfile(), pConfig->get_channel_prefix(), pConfig->get_channel_id(), pConfig->get_logsize(), pConfig->get_loglevel(), pConfig->get_impotant_info_marker());
    	logger::titr_log  = new logger::TTitrLog(pConfig->get_loghistory(), pConfig->get_channel_prefix(), pConfig->get_channel_id(), pConfig->get_loglevel());

  	logger::message << endl << endl << "Configuration file :  " << argv[1] << std::endl;
    	logger::write_log(logger::message);
    }
    else
    {
    	logger::message << ERR_CONFIG_FILE << " " << time(0) << " Something wrong with Configuration file " <<  argv[1] << std::endl;
    	logger::write_info(logger::message);

        cleaner(pConfig, pPlaylist, pParameters);
        return EXIT_FAILURE;
    }

    unsigned int time1 = common::mtime();
    pParameters = new Tparameters(pConfig->aac_streamtype(), pConfig->get_playlist(), pConfig->get_playfile(), pConfig->get_npfile(), pConfig->get_jingle_path());
    unsigned int time2 = common::mtime();

    logger::message.str("");
    logger::message << endl << endl << "TIME = " << time2 - time1 << endl << endl;
    logger::write_info(logger::message);

    if (!pParameters)
    {
    	logger::message.str("");
    	logger::message.str("Something wrong with pParameters = new Tparameters()\n");
    	logger::write_log(logger::message);

    	cleaner(pConfig, pPlaylist, pParameters);
        return EXIT_FAILURE;
    }


    std::ostringstream spl;

    if (pParameters->get_playfile_exists())
    {
              spl << time(0) << " Playing will be started with ";

              if (pParameters->get_playfile_default())
               {
                  handler_proc::query_playlist_detach(pConfig->get_url_get_playlist());
                  spl << "jingle ";
               }
              else  spl << "stored ";

              spl <<  "playlist sizeof " << pParameters->playlist_size() << " tracks" << endl;
              logger::write_marker(spl);
    }
    else
    {

                        logger::message <<  time(0) << "No stored or jingle playlist found" << endl;
                        logger::write_marker(logger::message);

                        handler_proc::handler_playlist_thread(pConfig->get_url_get_playlist());

			if (!pParameters->get_new_playlist())
			{
				cleaner(pConfig, pPlaylist, pParameters);
				return EXIT_FAILURE;
			}
			logger::message.str("");
    }

    /* запуск таймера на периодический запрос обновления плейлиста */
    //    handler_proc::playlist_timer_start(pConfig->get_query_playlist_interval());

    pPlaylist        = new TPlaylist(pConfig);
    if (pPlaylist->error())
    {
    	logger::message.str("");
    	logger::message << "\nSomething wrong with playlist " << pConfig->get_playlist() <<  std::endl;
    	logger::write_log(logger::message);

        cleaner(pConfig, pPlaylist, pParameters);
        return EXIT_FAILURE;
    }

    pPlaylist->start();
    if (pPlaylist->error())
    {
        logger::message.str("Finished On Error\n");
    	logger::write_log(logger::message);
    }

	logger::message.str("");
	logger::message << ERR_FINISHED << " " << time(0) << " Finished\n";
	logger::write_info(logger::message);

	cleaner(pConfig, pPlaylist, pParameters);

	return 0;
}

void Set_SIGUSR1()
{
	logger::message.str("");
	logger::message << time(0) << " Command Make received\n";
	logger::write_log(logger::message);

	if (pConfig && pParameters)
		handler_proc::query_playlist_detach(pConfig->get_url_get_playlist());
}

void Set_SIGDELETE()
{
    if (pPlaylist) { delete pPlaylist; pPlaylist = NULL; }

    logger::message.str("");
    logger::message << time(0) << " Command Stop/Exit received\n\n";
    logger::write_marker(logger::message);

    cleaner(pConfig, NULL, pParameters);
}

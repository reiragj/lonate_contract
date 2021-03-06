/**
 *  @file lonate.cpp
 *  @copyright defined in eos/LICENSE.txt
 *  @author    Geujnoo LEE, gjlee@lonate.io
 *  @date       2018-11-06
 *  @version  0.0.1
 */

// account_index accounts;
#include "lonate.hpp"
#include <eosiolib/print.hpp>
#include <eosiolib/transaction.hpp>
#include <cmath>


const double dPOWER = 2.00;     // cost for one curation
const double rPOwer = 0.00084;   //recovery rate per hour


void lonate::version()
{
    eosio::print("Lonate version  0.0.1");
};

void lonate::offer(eosio::name author, const eosio::asset &quantity, const std::string subject, const std::string context, const std::string category, const std::string pictures, const uint32_t endtime)
{
    require_auth(author);
    eosio_assert(endtime > now(), "invalid end time");

    eosio_assert(quantity.symbol == eosio::symbol("LWON", 4), "only core token allowed");
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must deposit positive quantity");

    offers.emplace(get_self(), [&](auto &proffer) {
        proffer.offerid = offers.available_primary_key();
        proffer.target_balance = quantity;
        proffer.author = author;
        proffer.subject = subject;
        proffer.context = context;
        proffer.category = category;
        proffer.endtime = endtime;
    });

    camcontrol(author, offers.available_primary_key()-1, endtime);
}

void lonate::comment(const eosio::name author, const uint64_t offerid, const std::string context)
{
    require_auth(author);
    auto cur_post_itr = offers.find(offerid);
    eosio_assert(cur_post_itr != offers.end(), "unknown post");
    auto new_comment_itr = comments.emplace(get_self(), [&](auto &comment) {
        comment.commentid = comments.available_primary_key();
        comment.offerid = offerid;
        comment.author = author;
        comment.context = context;
    });
}

void lonate::deposit(const eosio::name from, const eosio::asset &quantity)
{
    require_auth(from);
    eosio_assert(quantity.symbol == eosio::symbol("LWON", 4), "only LWON token allowed");
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must deposit positive quantity");

    auto itr = accounts.find(from.value);
    if (itr == accounts.end())
    {
        itr = accounts.emplace(get_self(), [&](auto &acnt) {
            acnt.owner = from;
        });
    }

    transfer(from, quantity);

    accounts.modify(itr, get_self(), [&](auto &acnt) {
        acnt.LWON_balance += quantity;
    });
}

void lonate::donate(const eosio::name from, const uint64_t postid, const eosio::asset &quantity)
{
    
    require_auth(from);
    eosio_assert(quantity.symbol == eosio::symbol("LWON", 4), "only core token allowed");
    eosio_assert(quantity.is_valid(), "invalid quantity");
    eosio_assert(quantity.amount > 0, "must deposit positive quantity");

    auto itr = accounts.find(from.value);
    if (itr == accounts.end())
    {
        itr = accounts.emplace(get_self(), [&](auto &acnt) {
            acnt.owner = from;
        });
    }
    auto acnt_itr = accounts.find(from.value);
    eosio_assert(acnt_itr != accounts.end(), "unknown account");

    auto post_itr = offers.find(postid);
    eosio_assert(post_itr != offers.end(), "unknown post");
    eosio_assert(post_itr->state == 0, "expired campagin");

    /*accounts.modify(acnt_itr, get_self(), [&](auto &acnt) {
        eosio_assert(acnt.LWON_balance >= quantity, "insufficient balance");
        acnt.LWON_balance -= quantity;
    });*/

    transfer(from, quantity);

    offers.modify(post_itr, get_self(), [&](auto &post) {
        post.offer_balance += quantity;
    });
}

void lonate::curate(const eosio::name from, const uint64_t offerid)
{
    auto acnt_itr = accounts.find(from.value);
    eosio_assert(acnt_itr != accounts.end(), "unknown account");
    eosio_assert(acnt_itr->vote_power >2, "not enough vote power");
    

    auto post_itr = offers.find(offerid);
    eosio_assert(post_itr != offers.end(), "unknown campagin");
    eosio_assert(post_itr->state == 0, "expired campagin");
    

    // update for user's vote power
    float vp = 100.00f;
    if( acnt_itr->vote_power != 100){
        vp = (float)acnt_itr->vote_power*(pow((float)(1+rPOwer), (int)((now() - acnt_itr->last_vote)/10)));
        vp = vp>100?100:vp;
    }

    // update for campaign
    float op = 1 - (((float)now() - (float)post_itr->created)/(((float)post_itr->endtime - (float)post_itr->created)))*0.5;



    
    if (from == post_itr->author)
    {
        //eosio_assert( now() - post.created < days(7), "cannot vote after 7 days" );

        offers.modify(post_itr, get_self(), [&](auto &post) {
            post.curated +=lonate::getcurpower(from);
            //post.offer_power = op*100;
            post.offer_power = op;
        });

        accounts.modify(acnt_itr, get_self(), [&](auto &acnt) {
            acnt.curate_cnt ++;
            acnt.last_vote = now();
            acnt.vote_power  = vp-dPOWER;
        });
    }
    /*else if (context == vote.voter)
    {
        offers.modify(post_itr, _self, [&](auto &post) {
            post.curated++;
        });

        accounts.modify(acnt_itr, _self, [&](auto &acnt) {
            acnt.curate_cnt++;
            acnt.last_vote = now();
            acnt.vote_power -= dPOWER;
        });
    }
    else
    {
        eosio_assert(false, "invalid context for execution of this vote");
    }*/
}

/*void checkvp(eosio::multi_index::const_iterator *acnt_itr){
    eosio::print(acnt_itr->last_vote);
}

void checkop(eosio::multi_index::const_iterator *acnt_itr){

}*/

void lonate::fulfill(eosio::name requestor, const uint64_t offerid, const eosio::asset &quantity, const std::string context){
     require_auth(requestor); 
     eosio_assert(quantity.symbol == eosio::symbol("LWON", 4), "only LWON token allowed");
     eosio_assert(quantity.is_valid(), "invalid quantity");
     eosio_assert(quantity.amount > 0, "must fulfill positive quantity");
     
     auto post_itr = offers.find(offerid);

     eosio_assert(post_itr != offers.end(), "unknown campagin");
     eosio_assert(post_itr->state == 1 , "expired campagin");
     eosio_assert(post_itr->success == 1, "collecting failed");
     eosio_assert(post_itr->author == requestor, "not allowed non owner");
     eosio_assert(post_itr->offer_balance.amount > quantity.amount, "not enough balance");

     eosio::action retire = eosio::action(
        eosio::permission_level{get_self(), "active"_n},
        "lonate.token"_n, "retire"_n,
        std::make_tuple(quantity, std::string("fulfillment list  TBD")));
    retire.send();

     offers.modify(post_itr, get_self(), [&](auto &post) {
        post.offer_balance -= quantity;
    });

    fulfillments.emplace(get_self(), [&](auto &fulfillment) {
        fulfillment.fulfillid = fulfillments.available_primary_key();
        fulfillment.requestor = requestor;
        fulfillment.quantity = quantity;
        fulfillment.context = context;
    });




} 

void lonate::deleterecord(const eosio::name from, const uint64_t offerid)
{

     /*auto acnt_itr = accounts.find(from.value);
     eosio_assert(acnt_itr != accounts.end(), "unknown account");

     accounts.modify(acnt_itr, get_self(), [&](auto &acnt) {
            acnt.vote_power  = 95;
        });

    auto post_itr = offers.find(offerid);

     offers.modify(post_itr, get_self(), [&](auto &post) {
            post.offer_power = 100;
        });*/
    uint64_t count = 0;
    for (auto itr = offers.begin(); itr != offers.end();)
    {
        // delete element and update iterator reference
        itr = offers.erase(itr);
        count++;
    }

}

void lonate::expire(eosio::name author, const uint64_t offerid)
{
    require_auth(get_self());
    require_recipient(author);

    auto offer_itr = offers.find(offerid);
    eosio_assert(offer_itr != offers.end(), "unknown campagin");

    offers.modify(offer_itr, get_self(), [&](auto &offer) {
        if(offer.offer_balance.amount*10/offer.target_balance.amount >=7){
            offer.success = 1;
        }
        offer.state++;
    });
}

void lonate::transfer(const eosio::name from, const eosio::asset &quantity)
{

    eosio::action transfer = eosio::action(
        eosio::permission_level{from, "active"_n},
        "lonate.token"_n, "transfer"_n,
        std::make_tuple(from, _self, quantity, std::string("donate")));
    transfer.send();
}

void lonate::camcontrol(const eosio::name author, const uint64_t offerid, const uint32_t end)
{
    eosio::transaction trx;
    trx.actions.emplace_back(eosio::permission_level{get_self(), "active"_n},
                             get_self(), "expire"_n,
                             std::make_tuple(author, offerid));
    trx.delay_sec = end - now();
    trx.send(offerid+now(), _self);
}

EOSIO_DISPATCH(lonate, (version)(offer)(comment)(deposit)(donate)(curate)(fulfill)(expire))